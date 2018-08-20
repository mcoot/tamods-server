#pragma once

#include "buildconfig.h"

#include <chrono>
#include <iostream>
#include <mutex>
#include <queue>

#include <boost/function.hpp>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <nlohmann/json.hpp>

#include "Logger.h"

using json = nlohmann::json;
using boost::asio::ip::tcp;

namespace TCP {

	// Client represents an asynchronous TCP Client
	template <typename SizeType>
	class Client {
		typedef std::function<void(const json&)> RecvHandlerType;
	private:
		boost::system::error_code error_state;
		bool stopped;
		boost::asio::deadline_timer deadline;
		std::vector<SizeType> read_msgsize_buff;
		std::mutex write_mutex;
		tcp::socket socket;
		std::map<short, RecvHandlerType> handlers;

	private:

		void start_connect(tcp::endpoint endpoint) {
			// Connection deadline of 60 seconds before it is considered a failure
			deadline.expires_from_now(boost::posix_time::seconds(60));

			socket.async_connect(endpoint, boost::bind(&Client::handle_connect, this, _1));
		}

		void handle_connect(const boost::system::error_code& conErr) {
			if (stopped) return;

			if (!socket.is_open()) {
				// async_connect automatically opens the socket
				// If the socket is still closed, then timeout occurred
				// Also check if in an error state
				stop();
				error_state = boost::system::error_code(boost::system::errc::connection_aborted, boost::system::generic_category());
				return;
			}
			else if (conErr) {
				// Connection error
				stop();
				error_state = conErr;
				return;
			}

			// Successful connection

			// No longer need a deadline
			deadline.expires_from_now(boost::posix_time::pos_infin);

			// Start the input actor
			start_read();
		}

		void start_read() {
			// Set up asynchronous read for the beginning of a message (first bytes being the message size)
			boost::asio::async_read(socket, boost::asio::buffer(read_msgsize_buff, sizeof(SizeType)), boost::asio::transfer_exactly(sizeof(SizeType)), boost::bind(&Client::handle_read, this, _1));
		}

		void handle_read(boost::system::error_code readErr) {
			if (stopped) return;
			if (readErr) {
				error_state = readErr;
				stop();
				return;
			}

			// Already read the message size
			SizeType msgSize = read_msgsize_buff[0];

			// Synchronously read and extract the message kind
			short msgKind;
			size_t syncBytesRead = boost::asio::read(socket, boost::asio::buffer(&msgKind, 2), readErr);
			if (!readErr && syncBytesRead != 2) {
				error_state = boost::system::error_code(boost::system::errc::message_size, boost::system::generic_category());
				stop();
				return;
			}
			if (readErr) {
				error_state = readErr;
				stop();
				return;
			}

			// Read the actual message
			std::vector<char> rawMsg(msgSize - 2);
			syncBytesRead = boost::asio::read(socket, boost::asio::buffer(rawMsg, msgSize - 2), readErr);
			if (syncBytesRead != msgSize - 2) {
				error_state = boost::system::error_code(boost::system::errc::message_size, boost::system::generic_category());
				stop();
				return;
			}
			if (!readErr && readErr) {
				error_state = readErr;
				stop();
				return;
			}
			std::string msgString = std::string(rawMsg.begin(), rawMsg.end());
			// Parse the message into json
			json msgJson;
			try {
				msgJson = json::parse(msgString);
			}
			catch (const json::parse_error&) {
				error_state = boost::system::error_code(boost::system::errc::bad_message, boost::system::generic_category());
				stop();
				return;
			}

			// Call the handler for this kind of message
			// If there is no handler, ignore the message
			auto& it = handlers.find(msgKind);
			if (it != handlers.end()) {
				it->second(msgJson);
			}

			// Begin the next asynchronous read
			start_read();
		}

		void write(short msgKind, const json& msgBody) {
			if (stopped) return;

			// Serialise message
			std::string msgString = msgBody.dump();
			SizeType msgSize = (SizeType)msgString.length() + sizeof(msgKind);

			// Construct the bytes to send
			std::ostringstream out_stream;
			out_stream.write(reinterpret_cast<char*>(&msgSize), sizeof(msgSize));
			out_stream.write(reinterpret_cast<char*>(&msgKind), sizeof(msgKind));
			out_stream << msgString;

			// Message writing is synchronous and serialised to avoid a data race
			boost::system::error_code writeErr;
			
			size_t bytesWritten;
			{
				std::lock_guard<std::mutex> lock(write_mutex);
				bytesWritten = boost::asio::write(socket, boost::asio::buffer(out_stream.str(), out_stream.str().length()), writeErr);
			}
			if (!writeErr && bytesWritten != msgSize + 2) {
				writeErr.assign(boost::system::errc::message_size, boost::system::generic_category());
			}
			if (writeErr) {
				error_state = writeErr;
				stop();
			}
		}

		void check_deadline() {
			if (stopped) return;

			// Check if the deadline has passed
			if (deadline.expires_at() <= boost::asio::deadline_timer::traits_type::now()) {
				// Kill the connection, anything outstanding will be cancelled
				socket.close();
				// Set infinite expiry since the deadline is complete
				deadline.expires_at(boost::posix_time::pos_infin);
			}

			// Put the actor back to sleep.
			deadline.async_wait(boost::bind(&Client::check_deadline, this));
		}

	public:
		Client(boost::asio::io_service& io_service) :
			socket(io_service),
			read_msgsize_buff(1),
			write_mutex(),
			deadline(io_service),
			error_state(),
			handlers(),
			stopped(false) {
			static_assert(std::is_integral<SizeType>::value, "SizeType type parameter must be an integral type");
		}

		void add_handler(short msgKind, RecvHandlerType handler) {
			handlers[msgKind] = handler;
		}

		void send(short msgKind, const json& msgBody) {
			write(msgKind, msgBody);
		}

		void start(std::string host, int port) {
			stopped = false;
			boost::asio::ip::address addr = boost::asio::ip::address::from_string(host, error_state);
			if (error_state) {
				return;
			}
			tcp::endpoint endpoint(addr, port);

			start_connect(endpoint);

			// Start the deadline actor
			// This doesn't actually set a specific deadline, that is set elsewhere
			deadline.async_wait(boost::bind(&Client::check_deadline, this));
		}

		void stop() {
			Logger::debug("stopping... error code is %d", error_state.value());
			stopped = true;
			socket.close();
			deadline.cancel();
		}

		boost::system::error_code get_error_state() {
			return error_state;
		}

		bool is_stopped() {
			return stopped;
		}

	};

}