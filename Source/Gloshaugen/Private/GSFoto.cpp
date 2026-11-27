// Grafikk.Foto (Grafikk og kart, runde 5a): a fixed camera route through Helgasetr in the running game, with
// screenshots and frame timings, that closes the game by itself. Run headless by Tools/Unreal/run_foto.ps1:
//   UnrealEditor.exe Gloshaugen.uproject /Game/Maps/Gloshaugen?Mode=Explore -game ... -ExecCmds="Grafikk.Foto"
// Per view: wait for Lumen and the eye adaptation to settle, measure frame / game / draw / GPU time for 3 s, log one
// line ("Grafikk.Foto: A kl12 frame 15.8/19.2 game 4.1 draw 6.3 gpu 14.9/17.0 ms") and take a HighResShot to
// Saved/Screenshots/foto/<view>_<time>.png. Same views as the Blender renders of round 4b (render_helgasetr.py).
// "Grafikk.Foto stopp" ends a run that is going on (without quitting).

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "RenderTimer.h"
#include "DynamicRHI.h"
#include "UnrealClient.h"
#include "Scalability.h"
#include "GSGameState.h"
#include "GSHelgasetrBuilder.h"

namespace GSFotoImpl
{
	struct FView
	{
		const TCHAR* Id;
		const TCHAR* Name;
		FVector PosM;
		FVector LookM;
		float Fov;   // horizontal, degrees (Blender lens 16 mm = 97, 22 mm = 79, 14 mm = 104 on a 36 mm sensor)
	};

	// Meters, game axes (X east, Y south, Z up). P4 eye height = 13.9 + 1.65.
	const FView Views[] = {
		{ TEXT("A"), TEXT("P4 nordkorridor mot sor"), FVector(9.5f, -18.9f, 15.55f), FVector(-2.f, 8.f, 12.55f), 97.f },
		{ TEXT("B"), TEXT("kantina opp mot glasstaket"), FVector(2.f, 4.f, 1.65f), FVector(0.f, -6.f, 24.f), 104.f },
		{ TEXT("C"), TEXT("mesaninen ned i kantina"), FVector(2.f, 16.f, 7.25f), FVector(0.f, -10.f, 0.5f), 97.f },
		{ TEXT("D"), TEXT("P4 vestkorridor"), FVector(-14.4f, 17.f, 15.55f), FVector(-14.2f, -17.f, 15.55f), 79.f },
		{ TEXT("E"), TEXT("rodt lags base fra slusa"), FVector(-1.2f, 22.4f, 15.6f), FVector(8.f, 27.5f, 14.6f), 90.f },
		{ TEXT("F"), TEXT("atriet ovenfra fra P4-rekkverket"), FVector(-13.f, 17.6f, 15.55f), FVector(2.f, -6.f, 3.f), 104.f },
		{ TEXT("G"), TEXT("grupperom 3.N2 pa P3"), FVector(12.6f, -17.3f, 11.5f), FVector(7.5f, -12.8f, 10.2f), 90.f },
	};

	enum class EKind : uint8 { Measure, StatShot, Straaling };
	struct FStep
	{
		int32 View;
		float Hour;
		EKind Kind = EKind::Measure;
		const TCHAR* Suffix = TEXT("");
	};

	struct FRun
	{
		TWeakObjectPtr<UWorld> World;
		TWeakObjectPtr<ACameraActor> Camera;
		TArray<FStep> Steps;
		int32 Step = -1;
		double StartedAt = 0.0;
		double PhaseStart = 0.0;
		int32 Phase = 0;   // 0 wait for the map, 1 settle, 2 measure, 4 screenshot asked, 3 next step
		float Hour = -1.f;
		bool bQuit = true;
		// measurements
		int32 Frames = 0;
		double SumFrame = 0, MaxFrame = 0, SumGame = 0, SumDraw = 0, SumGpu = 0, MaxGpu = 0;
		FString LastFile;
		int32 Shots = 0, ShotsMissing = 0;
		FTSTicker::FDelegateHandle Ticker;
	};

	TUniquePtr<FRun> GRun;

	constexpr float SettleSeconds = 3.f;
	constexpr float SettleAfterTimeChange = 5.f;
	constexpr float MeasureSeconds = 3.f;
	constexpr double MaxRunSeconds = 330.0;   // run_foto.ps1 kills the game at 6 min; quit on our own first

	UWorld* GameWorld()
	{
		if (!GEngine) return nullptr;
		for (const FWorldContext& C : GEngine->GetWorldContexts())
		{
			if ((C.WorldType == EWorldType::Game || C.WorldType == EWorldType::PIE) && C.World()) return C.World();
		}
		return nullptr;
	}

	FString FotoDir()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots") / TEXT("foto"));
	}

	FString HourTag(float H) { return FString::Printf(TEXT("kl%02d"), FMath::RoundToInt(H)); }

	APlayerController* LocalPC(UWorld* W)
	{
		for (FConstPlayerControllerIterator It = W->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* PC = It->Get())
			{
				if (PC->IsLocalController()) return PC;
			}
		}
		return nullptr;
	}

	void SetClock(UWorld* W, float Hour)
	{
		AGSGameState* GS = W->GetGameState<AGSGameState>();
		if (!GS) return;
		const float Now = GS->GetServerWorldTimeSeconds();
		GS->StartHour = Hour;
		GS->ClockStartedAt = Now;
		GS->Weather = EGSWeather::Clear;
		GS->PreviousWeather = EGSWeather::Clear;
		GS->WeatherChangedAt = Now - 1000.f;
		GS->HoldWeather(3600.f);
	}

	void PlaceCamera(FRun& R, const FView& V)
	{
		ACameraActor* Cam = R.Camera.Get();
		if (!Cam) return;
		const FVector P = V.PosM * 100.f;
		const FVector L = V.LookM * 100.f;
		Cam->SetActorLocationAndRotation(P, (L - P).Rotation());
		Cam->GetCameraComponent()->SetFieldOfView(V.Fov);
	}

	void ResetMeasure(FRun& R)
	{
		R.Frames = 0;
		R.SumFrame = R.MaxFrame = R.SumGame = R.SumDraw = R.SumGpu = R.MaxGpu = 0.0;
	}

	void Finish(FRun& R, const TCHAR* Why)
	{
		UE_LOG(LogTemp, Log, TEXT("Grafikk.Foto: ferdig (%s): %d bilder tatt, %d mangler, %.0f s"), Why, R.Shots, R.ShotsMissing,
			FPlatformTime::Seconds() - R.StartedAt);
		if (R.Shots == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Grafikk.Foto: ingen bilder (skjermen av eller PC-en låst?)"));
		}
		const bool bQuit = R.bQuit;
		FTSTicker::GetCoreTicker().RemoveTicker(R.Ticker);
		GRun.Reset();
		if (bQuit)
		{
			FPlatformMisc::RequestExit(false, TEXT("Grafikk.Foto"));
		}
	}

	bool Tick(float /*Dt*/)
	{
		if (!GRun.IsValid()) return false;
		FRun& R = *GRun;
		const double Now = FPlatformTime::Seconds();
		if (Now - R.StartedAt > MaxRunSeconds)
		{
			Finish(R, TEXT("tidsgrensen"));
			return false;
		}
		UWorld* W = GameWorld();
		if (!W)
		{
			return true;
		}

		if (R.Phase == 0)
		{
			// Wait until Helgasetr is built (the builder exists and a local player controller is there), at most 10 s
			// after the world is up; then hide everything that is not the picture.
			bool bBuilt = false;
			for (TActorIterator<AGSHelgasetrBuilder> It(W); It; ++It) bBuilt = true;
			APlayerController* PC = LocalPC(W);
			if (!PC || (!bBuilt && Now - R.PhaseStart < 10.0) || W->GetTimeSeconds() < 2.f) return true;
			if (!bBuilt) UE_LOG(LogTemp, Warning, TEXT("Grafikk.Foto: fant ingen AGSHelgasetrBuilder etter 10 s, fortsetter"));

			Scalability::FQualityLevels Q = Scalability::GetQualityLevels();
			Q.SetFromSingleQualityLevel(1);   // Middels (Lav, Middels, Hoy, Episk, Film), not saved
			Scalability::SetQualityLevels(Q, true);
			if (PC->MyHUD) PC->MyHUD->bShowHUD = false;
			if (UGameViewportClient* VC = W->GetGameViewport()) VC->RemoveAllViewportWidgets();
			if (GEngine) GEngine->Exec(W, TEXT("DisableAllScreenMessages"));
			if (APawn* Pawn = PC->GetPawn())
			{
				Pawn->SetActorHiddenInGame(true);
				Pawn->DisableInput(PC);
			}
			FActorSpawnParameters SP;
			SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ACameraActor* Cam = W->SpawnActor<ACameraActor>(FVector::ZeroVector, FRotator::ZeroRotator, SP);
			Cam->GetCameraComponent()->SetConstraintAspectRatio(false);
			R.Camera = Cam;
			PC->SetViewTargetWithBlend(Cam, 0.f);
			IFileManager::Get().MakeDirectory(*FotoDir(), true);
			UE_LOG(LogTemp, Log, TEXT("Grafikk.Foto: start, %d steg, grafikk Middels, bilder til %s"), R.Steps.Num(), *FotoDir());
			R.Step = -1;
			R.Phase = 3;   // go to the first step
		}

		if (R.Phase == 3)
		{
			// Next step
			if (!R.LastFile.IsEmpty())
			{
				// HighResShot writes on a later frame: count the file once the next step starts.
				if (IFileManager::Get().FileExists(*R.LastFile)) ++R.Shots;
				else if (Now - R.PhaseStart < 4.0) return true;
				else { ++R.ShotsMissing; UE_LOG(LogTemp, Warning, TEXT("Grafikk.Foto: bildet %s ble ikke skrevet"), *R.LastFile); }
				R.LastFile.Empty();
			}
			++R.Step;
			if (!R.Steps.IsValidIndex(R.Step))
			{
				Finish(R, TEXT("alle utsnitt"));
				return false;
			}
			const FStep& S = R.Steps[R.Step];
			if (S.Kind == EKind::Straaling)
			{
				if (APlayerController* PC = LocalPC(W)) PC->ConsoleCommand(TEXT("Straaling 0"));
				UE_LOG(LogTemp, Log, TEXT("Grafikk.Foto: Straaling 0 (programvare-Lumen)"));
				R.PhaseStart = Now;
				R.Phase = 3;
				return true;
			}
			// The clock is set again at every view, so the sun stands still for the whole route (a game day is 48 min).
			const bool bNewHour = S.Hour != R.Hour;
			R.Hour = S.Hour;
			SetClock(W, S.Hour);
			PlaceCamera(R, Views[S.View]);
			if (S.Kind == EKind::StatShot && GEngine) GEngine->Exec(W, TEXT("stat gpu"));
			// longer after a jump in time (sun, sky capture, Lumen)
			R.Phase = 1;
			R.PhaseStart = Now + (bNewHour ? (SettleAfterTimeChange - SettleSeconds) : 0.0);
			return true;
		}

		const FStep& S = R.Steps[R.Step];
		const FView& V = Views[S.View];
		if (R.Phase == 1)
		{
			if (Now - R.PhaseStart < SettleSeconds) return true;
			ResetMeasure(R);
			R.PhaseStart = Now;
			R.Phase = 2;
			return true;
		}
		if (R.Phase == 2)
		{
			const double FrameMs = FApp::GetDeltaTime() * 1000.0;
			const double GameMs = FPlatformTime::ToMilliseconds(GGameThreadTime);
			const double DrawMs = FPlatformTime::ToMilliseconds(GRenderThreadTime);
			const double GpuMs = FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles(0));
			++R.Frames;
			R.SumFrame += FrameMs; R.MaxFrame = FMath::Max(R.MaxFrame, FrameMs);
			R.SumGame += GameMs; R.SumDraw += DrawMs;
			R.SumGpu += GpuMs; R.MaxGpu = FMath::Max(R.MaxGpu, GpuMs);
			if (Now - R.PhaseStart < MeasureSeconds) return true;

			const double N = FMath::Max(1, R.Frames);
			const FString Tag = FString::Printf(TEXT("%s %s%s"), V.Id, *HourTag(S.Hour), S.Suffix);
			UE_LOG(LogTemp, Log, TEXT("Grafikk.Foto: %s frame %.1f/%.1f game %.1f draw %.1f gpu %.1f/%.1f ms (%d bilder, %s)"),
				*Tag, R.SumFrame / N, R.MaxFrame, R.SumGame / N, R.SumDraw / N, R.SumGpu / N, R.MaxGpu, R.Frames, V.Name);

			const FString File = FotoDir() / FString::Printf(TEXT("%s_%s%s.png"), V.Id, *HourTag(S.Hour), S.Suffix);
			IFileManager::Get().Delete(*File, false, true, true);
			if (S.Kind == EKind::StatShot)
			{
				FScreenshotRequest::RequestScreenshot(File, true, false);   // with the stat gpu text
			}
			else
			{
				GEngine->Exec(W, *FString::Printf(TEXT("HighResShot 1920x1080 filename=%s"), *File));
			}
			R.LastFile = File;
			R.PhaseStart = Now;
			R.Phase = 4;
			return true;
		}
		if (R.Phase == 4)
		{
			// one more frame so the screenshot request is taken with this view (and the stat text), then the next step
			if (Now - R.PhaseStart < 0.5) return true;
			if (S.Kind == EKind::StatShot && GEngine) GEngine->Exec(W, TEXT("stat gpu"));   // toggles it off
			R.Phase = 3;
			return true;
		}
		return true;
	}

	void Start(const TArray<FString>& Args, UWorld* /*World*/)
	{
		if (Args.Num() > 0 && Args[0].Equals(TEXT("stopp"), ESearchCase::IgnoreCase))
		{
			if (GRun.IsValid())
			{
				GRun->bQuit = false;
				Finish(*GRun, TEXT("stoppet"));
			}
			return;
		}
		if (GRun.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("Grafikk.Foto: kjører allerede"));
			return;
		}
		GRun = MakeUnique<FRun>();
		FRun& R = *GRun;
		R.bQuit = !(Args.Num() > 0 && Args[0].Equals(TEXT("bli"), ESearchCase::IgnoreCase));   // "Grafikk.Foto bli": stay open
		R.StartedAt = R.PhaseStart = FPlatformTime::Seconds();
		for (float Hour : { 12.f, 15.f })
		{
			for (int32 V = 0; V < UE_ARRAY_COUNT(Views); ++V)
			{
				R.Steps.Add({ V, Hour });
				if (Hour == 12.f && V <= 1) R.Steps.Add({ V, Hour, EKind::StatShot, TEXT("_statgpu") });
			}
		}
		R.Steps.Add({ 1, 22.f });
		R.Steps.Add({ 4, 22.f });
		// Software Lumen (Kamp's "Straaling 0"): A and B once more at noon.
		R.Steps.Add({ 0, 12.f, EKind::Straaling });
		R.Steps.Add({ 0, 12.f, EKind::Measure, TEXT("_straaling0") });
		R.Steps.Add({ 1, 12.f, EKind::Measure, TEXT("_straaling0") });
		R.Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&Tick), 0.f);
		UE_LOG(LogTemp, Log, TEXT("Grafikk.Foto: venter på Helgasetr"));
	}

	FAutoConsoleCommandWithWorldAndArgs GCmdFoto(TEXT("Grafikk.Foto"),
		TEXT("Fast kamerarute gjennom Helgasetr med bilder (Saved/Screenshots/foto) og tidsmaling; lukker spillet etterpa. ")
		TEXT("'Grafikk.Foto bli' lar spillet sta, 'Grafikk.Foto stopp' avbryter."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Start));
}
