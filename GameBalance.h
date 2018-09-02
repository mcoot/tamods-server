#pragma once

namespace GameBalance {

	struct TrDevice {
		// Ammo
		int MaxAmmoCount;
		int m_nCarriedAmmo;
		int m_nMaxCarriedAmmo;
		int m_nLowAmmoWarning;
		int ShotCost;

		// Reload
		float m_fReloadTime;
		float m_fPctTimeBeforeReload;

		// Accuracy / Reticule
		float m_fDefaultAccuracy;
		float m_fAccuracyLossOnJump;
		float m_fAccuracyLossOnWeaponSwitch;
		float m_fAccuracyLossOnShot;
		float m_fAccuracyLossMax;
		int m_nReticuleIndex;

	};

}