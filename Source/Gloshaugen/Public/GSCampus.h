#pragma once

#include "CoreMinimal.h"
#include "GSTypes.h"

/**
 * Layout of NTNU Gloshaugen, in METERS.
 * X = east, Y = south, origin = middle of Hovedbygningen.
 *
 * The map is the core around Hovedbygningen (330 x 265 m, see PlayArea()), made for Capture the Flag:
 * base west = Gamle fysikk, base east = Gamle kjemi, Hovedbygningen with Lysgaarden in the middle.
 * The two bases are the same size and the map is point-symmetric about (0, 10): turn it 180 degrees
 * and west becomes east (Gamle kjemi has Yaw 180 for that reason).
 * Three routes between the bases: north (path along the facade, Y = -30), middle (end doors of
 * Hovedbygningen at X = +-55 and the corridor through Lysgaarden) and south (passages at X = +-78,
 * the courtyard and the promenade at Y = 120).
 * The play area is closed all the way round by site fences (AGSCampusBuilder::BuildPerimeter).
 * Buildings with bBackdrop stand outside the play area as scenery (model only, no loot or furniture).
 *
 * NOTE: the layout is hand-made and approximate; it is checked by Tools/Blender/check_campus.py.
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
		bool bBackdrop = false;   // scenery outside the play area: model only, no loot, furniture or props
	};

	inline const TArray<FBuilding>& Buildings()
	{
		static const FLinearColor Stone(0.52f, 0.50f, 0.47f);
		static const FLinearColor Brick(0.48f, 0.27f, 0.21f);
		static const FLinearColor Concrete(0.68f, 0.67f, 0.64f);
		static const FLinearColor Brown(0.40f, 0.31f, 0.25f);

		static const TArray<FBuilding> List = {
			// Hovedbygningen (U shape). Replaced by SM_Hovedbygningen (Blender) when imported. Lysgaarden is the big hall on the ground floor.
			{ TEXT("Hovedbygningen"),          0.f,    0.f, 110.f, 28.f,  0.f, 22.f, 5.5f, Stone, true },
			{ TEXT("Hovedbygningen vest"),   -48.f,   34.f,  14.f, 40.f,  0.f, 18.f, 4.5f, Stone, true },
			{ TEXT("Hovedbygningen oest"),    48.f,   34.f,  14.f, 40.f,  0.f, 18.f, 4.5f, Stone, true },
			{ TEXT("Gamle fysikk"),         -125.f,   10.f,  46.f, 22.f,  0.f, 16.f, 4.f,  Brick, true },   // base vest
			{ TEXT("Gamle kjemi"),           125.f,   10.f,  46.f, 22.f, 180.f, 16.f, 4.f, Brick, true },   // base oest (turned 180: mirror image of the west base)
			{ TEXT("Kjelhuset"),             -62.f,  105.f,  22.f, 18.f,  0.f, 12.f, 4.f,  Brick, true },
			// Scenery outside the play area
			{ TEXT("Sentralbygg 1"),         -20.f,  190.f,  34.f, 22.f,  0.f, 52.f, 4.f,  Concrete, false, true },
			{ TEXT("Sentralbygg 2"),          60.f,  215.f,  40.f, 22.f,  0.f, 44.f, 4.f,  Concrete, false, true },
			{ TEXT("Bergbygget"),           -160.f,  185.f,  60.f, 34.f,  0.f, 18.f, 4.5f, Brick, false, true },
			{ TEXT("Elektrobygget"),         235.f,  160.f, 120.f, 46.f,  0.f, 20.f, 4.f,  Brown, false, true },
		};
		return List;
	}

	struct FRoad { FVector2D A, B; float Width; bool bPath; };

	inline const TArray<FRoad>& Roads()
	{
		static const TArray<FRoad> List = {
			// Hoegskoleringen (car road, north edge)
			{ FVector2D(-260.f, -110.f), FVector2D(260.f, -110.f), 9.f, false },
			// North route
			{ FVector2D(0.f, -110.f),    FVector2D(0.f, -27.f),    6.f, true },  // up to the main entrance
			{ FVector2D(-158.f, -30.f),  FVector2D(158.f, -30.f),  4.f, true },  // along the facade, base to base
			{ FVector2D(-125.f, -30.f),  FVector2D(-125.f, -3.f),  3.f, true },  // to the north door of each base
			{ FVector2D(125.f, -30.f),   FVector2D(125.f, -3.f),   3.f, true },
			// Middle route: end doors of Hovedbygningen and of the bases
			{ FVector2D(-78.f, 0.f),     FVector2D(-57.f, 0.f),    3.f, true },
			{ FVector2D(57.f, 0.f),      FVector2D(78.f, 0.f),     3.f, true },
			{ FVector2D(-100.f, 11.6f),  FVector2D(-78.f, 11.6f),  3.f, true },  // the end doors of the bases are 1.64 m off the middle
			{ FVector2D(78.f, 8.4f),     FVector2D(100.f, 8.4f),   3.f, true },
			// South route
			{ FVector2D(-78.f, -30.f),   FVector2D(-78.f, 120.f),  4.f, true },  // west passage
			{ FVector2D(78.f, -30.f),    FVector2D(78.f, 120.f),   4.f, true },  // east passage
			{ FVector2D(-200.f, 120.f),  FVector2D(200.f, 120.f),  5.f, true },  // south promenade
			{ FVector2D(-125.f, 23.f),   FVector2D(-125.f, 120.f), 3.f, true },  // from the south door of each base
			{ FVector2D(125.f, 23.f),    FVector2D(125.f, 120.f),  3.f, true },
			{ FVector2D(0.f, 58.f),      FVector2D(0.f, 170.f),    5.f, true },  // from the courtyard towards Sentralbygg 1
		};
		return List;
	}

	/** The part of campus the match is played on (m). Everything outside is scenery. */
	inline FBox2D PlayArea() { return FBox2D(FVector2D(-165.f, -125.f), FVector2D(165.f, 140.f)); }

	/** Capture the Flag bases. Index 0 = west, 1 = east. Flag and spawn positions in meters.
	 *  The spawns lie behind the base building (between it and the site fence), out of sight from all three routes. */
	struct FBase
	{
		const TCHAR* Name;
		const TCHAR* Building;
		FVector Flag;            // on the ground floor
		float SpawnYaw;          // degrees, facing the middle of the map
		TArray<FVector2D> Spawns;
	};

	inline const TArray<FBase>& Bases()
	{
		static const TArray<FBase> List = {
			{ TEXT("Vest"), TEXT("Gamle fysikk"), FVector(-125.f, 10.f, 0.f), 0.f,
			  { { -161.f, 3.f }, { -161.f, 10.f }, { -161.f, 17.f }, { -155.f, 3.f }, { -155.f, 10.f }, { -155.f, 17.f }, { -158.f, 6.5f } } },
			{ TEXT("Oest"), TEXT("Gamle kjemi"), FVector(125.f, 10.f, 0.f), 180.f,
			  { { 161.f, 17.f }, { 161.f, 10.f }, { 161.f, 3.f }, { 155.f, 17.f }, { 155.f, 10.f }, { 155.f, 3.f }, { 158.f, 13.5f } } },
		};
		return List;
	}

	/** Outdoor spawn points (m): the base spawns plus four in front of and south of Hovedbygningen, all inside PlayArea(). */
	inline const TArray<FVector2D>& SpawnPoints()
	{
		static const TArray<FVector2D> List = {
			{ -161.f, 3.f }, { -161.f, 10.f }, { -161.f, 17.f }, { -155.f, 3.f }, { -155.f, 10.f }, { -155.f, 17.f }, { -158.f, 6.5f },
			{ 161.f, 17.f }, { 161.f, 10.f }, { 161.f, 3.f }, { 155.f, 17.f }, { 155.f, 10.f }, { 155.f, 3.f }, { 158.f, 13.5f },
			{ -60.f, -80.f }, { 60.f, -80.f }, { -20.f, 125.f }, { 20.f, 125.f },
		};
		return List;
	}

}
