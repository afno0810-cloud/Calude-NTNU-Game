#pragma once

#include "CoreMinimal.h"
#include "GSTypes.h"

/**
 * Layout of NTNU Gloshaugen, in METERS.
 * X = east, Y = south, origin = middle of Hovedbygningen.
 *
 * NOTE: this is an approximate hand-made layout (blockout). It will be replaced by real
 * OpenStreetMap footprints + Kartverket terrain when that data is available.
 */
namespace GSCampus
{
	struct FBuilding
	{
		const TCHAR* Name;
		float X, Y;          // center (m)
		float SizeX, SizeY;  // footprint (m)
		float Yaw;           // degrees
		float Height;        // m
		float FloorHeight;   // m
		FLinearColor Color;
		bool bEnterable;     // hollow with floors, ramps, windows and doors
	};

	inline const TArray<FBuilding>& Buildings()
	{
		static const FLinearColor Stone(0.52f, 0.50f, 0.47f);
		static const FLinearColor Brick(0.48f, 0.27f, 0.21f);
		static const FLinearColor Concrete(0.68f, 0.67f, 0.64f);
		static const FLinearColor Light(0.74f, 0.71f, 0.64f);
		static const FLinearColor Brown(0.40f, 0.31f, 0.25f);
		static const FLinearColor Glass(0.40f, 0.50f, 0.55f);

		static const TArray<FBuilding> List = {
			// Hovedbygningen (U shape) + tower behind it. Replaced by SM_Hovedbygningen (Blender) when imported. Lysgaarden is the big hall on the ground floor.
			{ TEXT("Hovedbygningen"),          0.f,    0.f, 110.f, 28.f,  0.f, 22.f, 5.5f, Stone, true },
			{ TEXT("Hovedbygningen vest"),   -48.f,   34.f,  14.f, 40.f,  0.f, 18.f, 4.5f, Stone, true },
			{ TEXT("Hovedbygningen oest"),    48.f,   34.f,  14.f, 40.f,  0.f, 18.f, 4.5f, Stone, true },
			{ TEXT("Hovedbygningen taarn"),    0.f,   20.f,  12.f, 12.f,  0.f, 42.f, 4.f,  Stone, false },
			{ TEXT("Gamle fysikk"),         -125.f,   10.f,  46.f, 22.f,  0.f, 16.f, 4.f,  Brick, true },
			{ TEXT("Gamle kjemi"),           125.f,   10.f,  56.f, 22.f,  0.f, 16.f, 4.f,  Brick, true },
			{ TEXT("Kjelhuset"),             -62.f,  105.f,  22.f, 18.f,  0.f, 12.f, 4.f,  Brick, true },
			{ TEXT("Sentralbygg 1"),         -20.f,  190.f,  34.f, 22.f,  0.f, 52.f, 4.f,  Concrete, true },
			{ TEXT("Sentralbygg 2"),          60.f,  215.f,  40.f, 22.f,  0.f, 44.f, 4.f,  Concrete, true },
			{ TEXT("Bergbygget"),           -160.f,  185.f,  60.f, 34.f,  0.f, 18.f, 4.5f, Brick, true },
			{ TEXT("Elektrobygget"),         235.f,  160.f, 120.f, 46.f,  0.f, 20.f, 4.f,  Brown, true },
			{ TEXT("IT-bygget"),             370.f,  105.f,  70.f, 26.f,  0.f, 24.f, 4.f,  Light, true },
			{ TEXT("Stripa"),                105.f,  305.f, 210.f, 18.f,  0.f,  9.f, 4.5f, Glass, true },
			{ TEXT("Kjemiblokkene"),        -105.f,  335.f, 140.f, 40.f,  0.f, 18.f, 4.5f, Light, true },
			{ TEXT("Varmeteknisk"),          370.f,  280.f,  70.f, 40.f,  0.f, 20.f, 4.f,  Concrete, true },
			{ TEXT("Realfagbygget"),         235.f,  425.f, 160.f, 70.f,  0.f, 22.f, 4.4f, Light, true },
			{ TEXT("Idrettsbygget"),        -240.f,  470.f,  80.f, 55.f,  0.f, 12.f, 6.f,  Glass, true },
		};
		return List;
	}

	struct FRoad { FVector2D A, B; float Width; bool bPath; };

	inline const TArray<FRoad>& Roads()
	{
		static const TArray<FRoad> List = {
			// Hoegskoleringen (car roads)
			{ FVector2D(-330.f, -110.f), FVector2D(470.f, -110.f), 9.f, false },
			{ FVector2D(470.f, -110.f),  FVector2D(470.f, 560.f),  9.f, false },
			{ FVector2D(-330.f, -110.f), FVector2D(-330.f, 560.f), 9.f, false },
			{ FVector2D(-330.f, 560.f),  FVector2D(470.f, 560.f),  9.f, false },
			{ FVector2D(150.f, 80.f),    FVector2D(470.f, 80.f),   7.f, false },
			// Walking paths
			{ FVector2D(0.f, -110.f),    FVector2D(0.f, -27.f),    6.f, true },  // up to the main entrance
			{ FVector2D(0.f, 55.f),      FVector2D(0.f, 560.f),    5.f, true },  // central axis south
			{ FVector2D(-330.f, 120.f),  FVector2D(470.f, 120.f),  4.f, true },
			{ FVector2D(-330.f, 260.f),  FVector2D(470.f, 260.f),  4.f, true },
			{ FVector2D(150.f, 330.f),   FVector2D(150.f, 560.f),  4.f, true },
			{ FVector2D(-200.f, -110.f), FVector2D(-200.f, 560.f), 4.f, true },
		};
		return List;
	}

	/** Outdoor spawn points (m). */
	inline const TArray<FVector2D>& SpawnPoints()
	{
		static const TArray<FVector2D> List = {
			{ -60.f, -80.f }, { 60.f, -80.f }, { -250.f, -60.f }, { 300.f, -60.f },
			{ -260.f, 120.f }, { 180.f, 100.f }, { 420.f, 190.f }, { 20.f, 120.f },
			{ -260.f, 260.f }, { 240.f, 260.f }, { -40.f, 420.f }, { 100.f, 520.f },
			{ 400.f, 520.f }, { -300.f, 380.f }, { 330.f, 360.f }, { -140.f, 520.f },
		};
		return List;
	}

}
