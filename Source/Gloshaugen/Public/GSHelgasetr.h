#pragma once

#include "CoreMinimal.h"

/**
 * Helgasetr (NTNU, Elgeseter gate 10): the map data, in METERS. X = east, Y = south, Z = up,
 * origin = axis 5 x axis H on the Plan 1 floor. Plan 1-4 are playable (Z 0 / 5.6 / 9.9 / 13.9), Plan 5-6 are scenery.
 *
 * Everything comes from Tools/Blender/helgasetr_plan.py through the generated GSHelgasetr.gen.inl
 * (python Tools/Blender/gen_helgasetr.py), checked by Tools/Blender/check_helgasetr.py.
 * The map description (and the "why") is .claude/lag/grafikk/helgasetr.md.
 *
 * Ids are stable: DoorSlots(), GlassPanels(), BreachWalls() and Cover() are numbered in list order, and
 * AGSHelgasetrBuilder puts instance N of each group at id N.
 * Do not include GSCampus.h here (GSCampus.h includes this file).
 */
namespace GSHelgasetr
{
	struct FRectM { float X0, Y0, X1, Y1; };
	struct FPointM { float X, Y; };
	struct FSegM { float AX, AY, BX, BY; };
	struct FZoneM { const TCHAR* Name; float X0, Y0, X1, Y1; };
	struct FLevelRectM { uint8 Level; float X0, Y0, X1, Y1; };

	/** How a structure box collides. */
	enum ECollM : uint8
	{
		Coll_Solid = 0,     // blocks everything
		Coll_High = 1,      // over Plan 4 in the atrium: blocks Visibility/Camera only (no standing, no grapple hook, K10)
		Coll_None = 2,
		Coll_Blocker = 3,   // invisible: blocks Pawn and the WorldStatic channel (edge of the atrium on Plan 5-6)
	};

	/** Round 4b: how a Blender model part collides (Tools/Blender/helgasetr_plan.py MODEL_COLL_NAMES). */
	enum EModelCollM : uint8
	{
		ModelColl_Full = 0,         // complex-as-simple, blocks everything
		ModelColl_GlassFull = 1,    // fixed glass (facade, roof): blocks everything, no shadow / distance field / ray tracing
		ModelColl_Above = 2,        // over Plan 4 in the atrium: Visibility/Camera only (K10)
		ModelColl_GlassAbove = 3,   // glass over Plan 4 in the atrium: Visibility/Camera only, no shadow
		ModelColl_None = 4,         // decoration (lamellas, rings, downlights): no collision, no distance field
	};
	struct FModelPartM { const TCHAR* Name; uint8 Coll; };

	/** A box: centre and size (m), rotation (deg), material index into MatNames, ECollM, bit index into BitNames. */
	struct FBoxM { float Cx, Cy, Cz, Sx, Sy, Sz, Yaw, Pitch, Roll; uint8 Mat; uint8 Coll; uint8 Bit; };
	/** Flag (m) on the floor of its base room; spawns are SpawnsM[FirstSpawn .. FirstSpawn + NumSpawns). */
	struct FBaseM { const TCHAR* Name; const TCHAR* Room; float X, Y, Z, SpawnYaw; int32 FirstSpawn, NumSpawns; };
	/** Breakable glass panel: centre (m), length along Yaw, height, thickness. Type 0 = rekkverk, 1 = vegg. */
	struct FPanelM { int32 Id; float Cx, Cy, Cz, Len, Height, Thick, Yaw; uint8 Type; uint8 Level; };
	/** Breach wall (lettvegg) or cover piece: an axis-aligned box (m). */
	struct FPieceM { int32 Id; float X0, Y0, Z0, X1, Y1, Z1; uint8 Mat; uint8 Level; const TCHAR* Name; };
	/** Door opening: floor point in the middle of the opening (m), width/height (m), Yaw = the way the front faces.
	 *  Kind indexes DoorKindNames (tre / glass_kode / dobbel / brann / laast). bLocked = locked at match start (all code
	 *  locks and the alarm door). bNoOpening = a sealed slot: the wall is whole and no door belongs there (the alarm door
	 *  to Trapp sør on Plan 4; the id is kept so no other id moves). */
	struct FDoorM { int32 Id; float X, Y, Z, Width, Height, Yaw; bool bHingeLeft; uint8 Kind; bool bLocked; bool bOpen; uint8 Level; const TCHAR* Room; bool bNoOpening; };
	/** Tier 0 common, 1 better, 2 crate. */
	struct FLootM { float X, Y, Z; uint8 Tier; };
	/** Kind 0 PC, 1 3D printer. */
	struct FTerminalM { float X, Y, Z, Yaw; uint8 Kind; const TCHAR* Name; };
	struct FCameraM { float X, Y, Z, Yaw; const TCHAR* Name; };
	/** Room acoustics: a box per area with its reverberation time (s). */
	struct FSoundZoneM { const TCHAR* Name; float X0, Y0, Z0, X1, Y1, Z1, Rt60; };
	/** Kind 0 = big hanging ring lamp, 1 = corridor light (attenuation RadiusM), 2 = ceiling spot (base lobbies).
	 *  bShadow: one of the at most 4 indoor lights that cast shadows (helgasetr.md kap. 9). */
	struct FLightM { float X, Y, Z; uint8 Kind; float RadiusM; bool bShadow; };
	struct FSignM { float X, Y, Z, Yaw; const TCHAR* Text; uint8 Team; };

#include "GSHelgasetr.gen.inl"

	inline TConstArrayView<FZoneM> Zones() { return MakeArrayView(ZonesM); }
	inline TConstArrayView<FBaseM> Bases() { return MakeArrayView(BasesM); }
	inline TConstArrayView<FPointM> Spawns() { return MakeArrayView(SpawnsM); }
	inline TConstArrayView<FBoxM> Boxes() { return MakeArrayView(BoxesM); }
	/** Model parts, and per box (same index as Boxes()) the part that replaces it (255 = always a box). */
	inline TConstArrayView<FModelPartM> ModelParts() { return MakeArrayView(ModelPartsM); }
	inline TConstArrayView<uint8> BoxParts() { return MakeArrayView(BoxPartM); }
	inline TConstArrayView<FPanelM> GlassPanels() { return MakeArrayView(PanelsM); }
	inline TConstArrayView<FPieceM> BreachWalls() { return MakeArrayView(BreachWallsM); }
	inline TConstArrayView<FPieceM> Cover() { return MakeArrayView(CoverM); }
	inline TConstArrayView<FDoorM> DoorSlots() { return MakeArrayView(DoorsM); }
	inline TConstArrayView<FLootM> LootSpots() { return MakeArrayView(LootM); }
	inline TConstArrayView<FTerminalM> Terminals() { return MakeArrayView(TerminalsM); }
	inline TConstArrayView<FCameraM> Cameras() { return MakeArrayView(CamerasM); }
	inline TConstArrayView<FSoundZoneM> SoundZones() { return MakeArrayView(SoundZonesM); }
	inline TConstArrayView<FLightM> Lights() { return MakeArrayView(LightsM); }
	inline TConstArrayView<FSignM> Signs() { return MakeArrayView(SignsM); }
	inline TConstArrayView<FSegM> Corridors() { return MakeArrayView(CorridorsM); }

	/** Floor rectangles (m) of one level (0 = Plan 1), for minimaps and checks. */
	inline TArray<FRectM> FloorRects(int32 Level)
	{
		TArray<FRectM> Out;
		for (const FLevelRectM& R : FloorRectsM)
		{
			if (R.Level == Level) Out.Add({ R.X0, R.Y0, R.X1, R.Y1 });
		}
		return Out;
	}

	/** The level a height (m) belongs to: the highest floor at most 0.3 m above it. 0 = Plan 1. */
	inline int32 LevelAt(float ZM)
	{
		int32 Best = 0;
		for (int32 L = 0; L < NumLevels; ++L)
		{
			if (LevelZM[L] <= ZM + 0.3f) Best = L;
		}
		return Best;
	}

	/** "1. etasje" ... "6. etasje" (Norwegian floor numbering = Plan number). */
	inline FString LevelName(int32 Level) { return FString::Printf(TEXT("%d. etasje"), FMath::Clamp(Level, 0, NumLevels - 1) + 1); }

	/** Name of the zone a point (m) lies in ("Atriet", "Helgasetr sørfløy" ...), or nullptr outside the building. */
	inline const TCHAR* ZoneAt(const FVector2D& P)
	{
		for (const FZoneM& Z : ZonesM)
		{
			if (P.X >= Z.X0 && P.X <= Z.X1 && P.Y >= Z.Y0 && P.Y <= Z.Y1) return Z.Name;
		}
		return nullptr;
	}

	/** Everything walkable (m): Plan 1 to just under the Plan 5 slab, inside the play area (for the bots' nav mesh). */
	inline FBox NavBoundsM()
	{
		return FBox(FVector(PlayAreaM.X0, PlayAreaM.Y0, -1.f), FVector(PlayAreaM.X1, PlayAreaM.Y1, LevelZM[4] - SlabM - 0.3f));
	}
}
