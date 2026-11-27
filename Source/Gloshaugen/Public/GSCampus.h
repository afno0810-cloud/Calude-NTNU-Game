#pragma once

#include "CoreMinimal.h"
#include "GSTypes.h"
#include "GSHelgasetr.h"

/**
 * THE MAP IS HELGASETR (27 Nov 2026): GSCampus::bHelgasetr = true makes Buildings(), Roads(), PlayArea(), Bases() and
 * SpawnPoints() return Helgasetr (data in GSHelgasetr.h, generated from Tools/Blender/helgasetr_plan.py), and
 * UGSWorldSubsystem builds it with AGSHelgasetrBuilder. Buildings() = five zones (Helgasetr nordfløy/sørfløy/vestfløy/
 * østfløy and Atriet) with storey height 4.3 m, so "N. etasje" is right for all six plans; bEnterable = false, so the old
 * random loot/terminals per building are not placed (read GSHelgasetr::LootSpots()/Terminals()/Cameras() instead).
 * Roads() = the ring corridor on Plan 4 (for the minimap).
 * The old Gloshaugen lists below are kept unchanged (Tools/Blender/check_campus.py reads them) until Andreas has
 * played Helgasetr and said yes.
 *
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
	/** true: the map is Helgasetr (indoor CTF). false: the old Gloshaugen campus. */
	inline constexpr bool bHelgasetr = true;

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
		if constexpr (bHelgasetr)
		{
			static const TArray<FBuilding> Zones = []()
			{
				TArray<FBuilding> Out;
				const FLinearColor Wall(0.72f, 0.70f, 0.66f);
				for (const GSHelgasetr::FZoneM& Z : GSHelgasetr::Zones())
				{
					// Height = the glass roof, FloorHeight 4.3: floor((Z + 0.25) / 4.3) + 1 gives 1..6 for the six plans.
					Out.Add({ Z.Name, (Z.X0 + Z.X1) * 0.5f, (Z.Y0 + Z.Y1) * 0.5f, Z.X1 - Z.X0, Z.Y1 - Z.Y0, 0.f,
						GSHelgasetr::RoofZM, 4.3f, Wall, false, false });
				}
				return Out;
			}();
			return Zones;
		}
		else
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
	}

	struct FRoad { FVector2D A, B; float Width; bool bPath; };

	inline const TArray<FRoad>& Roads()
	{
		if constexpr (bHelgasetr)
		{
			static const TArray<FRoad> Ring = []()
			{
				TArray<FRoad> Out;
				for (const GSHelgasetr::FSegM& S : GSHelgasetr::Corridors())
				{
					Out.Add({ FVector2D(S.AX, S.AY), FVector2D(S.BX, S.BY), 2.3f, true });
				}
				return Out;
			}();
			return Ring;
		}
		else
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
	}

	/** The part of campus the match is played on (m). Everything outside is scenery. */
	inline FBox2D PlayArea()
	{
		if constexpr (bHelgasetr)
		{
			const GSHelgasetr::FRectM& A = GSHelgasetr::PlayAreaM;
			return FBox2D(FVector2D(A.X0, A.Y0), FVector2D(A.X1, A.Y1));
		}
		else
		{
			return FBox2D(FVector2D(-165.f, -125.f), FVector2D(165.f, 140.f));
		}
	}

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
		if constexpr (bHelgasetr)
		{
			// Index 0 = Sørbasen (red), 1 = Nordbasen (blue), both on Plan 4 (floor 13.9 m).
			// Flag.Z = 13.9 = the floor. AGSGameMode (SpawnFlags, TryPlace) traces with an object trace from +2 m (15.9 m,
			// under the Plan 5 slab at 17.85 m) to -3 m. AGSHelgasetrBuilder::ProbeGameModeTraces runs the same traces
			// and logs whether both flags and all spawn points land on the floor.
			static const TArray<FBase> HBases = []()
			{
				TArray<FBase> Out;
				for (const GSHelgasetr::FBaseM& B : GSHelgasetr::Bases())
				{
					FBase Base{ B.Name, B.Room, FVector(B.X, B.Y, GSHelgasetr::FlagZBridgeM), B.SpawnYaw, {} };
					for (int32 I = 0; I < B.NumSpawns; ++I)
					{
						const GSHelgasetr::FPointM& S = GSHelgasetr::Spawns()[B.FirstSpawn + I];
						Base.Spawns.Add(FVector2D(S.X, S.Y));
					}
					Out.Add(Base);
				}
				return Out;
			}();
			return HBases;
		}
		else
		{
			static const TArray<FBase> List = {
				{ TEXT("Vest"), TEXT("Gamle fysikk"), FVector(-125.f, 10.f, 0.f), 0.f,
				  { { -161.f, 3.f }, { -161.f, 10.f }, { -161.f, 17.f }, { -155.f, 3.f }, { -155.f, 10.f }, { -155.f, 17.f }, { -158.f, 6.5f } } },
				{ TEXT("Oest"), TEXT("Gamle kjemi"), FVector(125.f, 10.f, 0.f), 180.f,
				  { { 161.f, 17.f }, { 161.f, 10.f }, { 161.f, 3.f }, { 155.f, 17.f }, { 155.f, 10.f }, { 155.f, 3.f }, { 158.f, 13.5f } } },
			};
			return List;
		}
	}

	/** Turn a point 180 degrees about (0, 10): the west half of the map becomes the east half. */
	inline FVector2D Mirror(const FVector2D& P) { return FVector2D(-P.X, 20.f - P.Y); }

	/** Tarpaulin fences of the rig area behind the WEST base (m), A -> B. The east base gets the same, Mirror()ed.
	 *  Two fences run from the edge fence (0.5 m inside the play area) towards the outer corners of the base building.
	 *  The 4 m openings next to the building are sluices: a second fence 2.5 m further out and a short closing leg
	 *  against the building wall, so you walk out sideways (towards the edge) and nobody on a path can look in.
	 *  The legs end 0.2 m inside the 0.4 m thick wall. Checked by Tools/Blender/check_campus.py g) and h). */
	struct FFence { FVector2D A, B; };

	inline const TArray<FFence>& RigFences()
	{
		static const TArray<FFence> List = {
			{ FVector2D(-164.5f, -3.f),  FVector2D(-152.f, -3.f) },    // north side of the rig
			{ FVector2D(-157.f, -5.5f),  FVector2D(-146.f, -5.5f) },   // north sluice
			{ FVector2D(-146.f, -5.5f),  FVector2D(-146.f, -0.8f) },   // its leg against the north wall
			{ FVector2D(-164.5f, 23.f),  FVector2D(-152.f, 23.f) },    // south side of the rig
			{ FVector2D(-157.f, 25.5f),  FVector2D(-146.f, 25.5f) },   // south sluice
			{ FVector2D(-146.f, 25.5f),  FVector2D(-146.f, 20.8f) },   // its leg against the south wall
		};
		return List;
	}

	/** Outdoor spawn points (m): the base spawns plus four in front of and south of Hovedbygningen, all inside PlayArea(). */
	inline const TArray<FVector2D>& SpawnPoints()
	{
		if constexpr (bHelgasetr)
		{
			static const TArray<FVector2D> HSpawns = []()
			{
				TArray<FVector2D> Out;
				for (const GSHelgasetr::FPointM& S : GSHelgasetr::Spawns()) Out.Add(FVector2D(S.X, S.Y));
				return Out;
			}();
			return HSpawns;
		}
		else
		{
			static const TArray<FVector2D> List = {
				{ -161.f, 3.f }, { -161.f, 10.f }, { -161.f, 17.f }, { -155.f, 3.f }, { -155.f, 10.f }, { -155.f, 17.f }, { -158.f, 6.5f },
				{ 161.f, 17.f }, { 161.f, 10.f }, { 161.f, 3.f }, { 155.f, 17.f }, { 155.f, 10.f }, { 155.f, 3.f }, { 158.f, 13.5f },
				{ -60.f, -80.f }, { 60.f, -80.f }, { -20.f, 125.f }, { 20.f, 125.f },
			};
			return List;
		}
	}

}
