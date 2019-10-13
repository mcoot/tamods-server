#include "TenantedDataStore.h"

using namespace TenantedDataStore;

DataMap<long long, PlayerSpecificData> TenantedDataStore::playerData;
DataMap<int, ClassSpecificData> TenantedDataStore::classData;