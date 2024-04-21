// Copyright Narrative Tools 2022. 


#include "NarrativeDialogueSequence.h"
#include <LevelSequenceActor.h>
#include <CineCameraActor.h>
#include <DefaultLevelSequenceInstanceData.h>
#include <CineCameraComponent.h>
#include "Dialogue.h"
#include <Engine/TargetPoint.h>

static const FName NAME_AnchorTag("Anchor");
static const FName NAME_CinecamTag("Cinecam");
static const FName NAME_NarrativeFaceTag("Face");
static const FName NAME_NarrativeBodyTag("Body");

UNarrativeDialogueSequence::UNarrativeDialogueSequence()
{
	bShouldRestart = false;
	AnchorOriginRule = EAnchorOriginRule::AOR_Speaker;
	AnchorRotationRule = EAnchorRotationRule::ARR_AnchorActorForwardVector;
	
	bUse180DegreeRule = true;
	UnitsY180DegreeRule = 9.f;
	DegreesYaw180DegreeRule = 20.f;
	bDrawDebugFocusPoint = false;

	CropSettings.AspectRatio = 2.35f;

	PlaybackSettings.bPauseAtEnd = true;
	PlaybackSettings.LoopCount.Value = 1;
	PlaybackSettings.PlayRate = 1.f;
	PlaybackSettings.StartTime = 0.f;
	PlaybackSettings.bRandomStartTime = false;
	PlaybackSettings.bHidePlayer = false;
	PlaybackSettings.bHideHud = true;
	PlaybackSettings.bDisableCameraCuts = false;
}

void UNarrativeDialogueSequence::Tick(const float DeltaTime)
{
	//Update the relative offset every frame if setting is enabled  
	if ((LookAtTrackingSettings.bUpdateTrackingEveryFrame || FocusTrackingSettings.bUpdateTrackingEveryFrame) && Dialogue && SequenceActor && SequenceActor->SequencePlayer)
	{
		if (Cinecam)
		{
			if (UCineCameraComponent* CinecamComp = Cinecam->GetCineCameraComponent())
			{
				if (LookAtTrackingSettings.AvatarToTrack > EShotTrackingRule::STR_Disabled && LookAtActor)
				{
					const FVector Offset = LookAtActor->GetActorTransform().InverseTransformPosition(Dialogue->GetSpeakerHeadLocation(LookAtActor));
					Cinecam->LookatTrackingSettings.RelativeOffset = FMath::VInterpTo(Cinecam->LookatTrackingSettings.RelativeOffset, Offset + LookAtTrackingSettings.TrackBoneNudge, DeltaTime, LookAtTrackingSettings.UpdateTrackingInterpSpeed);
				}

				if (FocusTrackingSettings.AvatarToTrack > EShotTrackingRule::STR_Disabled && FocusActor)
				{
					const FVector Offset = FocusActor->GetActorTransform().InverseTransformPosition(Dialogue->GetSpeakerHeadLocation(FocusActor));
					CinecamComp->FocusSettings.TrackingFocusSettings.RelativeOffset = FMath::VInterpTo(CinecamComp->FocusSettings.TrackingFocusSettings.RelativeOffset, Offset + FocusTrackingSettings.TrackBoneNudge, DeltaTime, FocusTrackingSettings.UpdateTrackingInterpSpeed);
				}
			}
		}
	}
}

void UNarrativeDialogueSequence::BeginPlaySequence(class ALevelSequenceActor* InSequenceActor, class UDialogue* InDialogue, class AActor* InSpeaker, class AActor* InListener)
{
	if (InSpeaker && InDialogue && InSequenceActor)
	{
		Speaker = InSpeaker;
		Listener = InListener;
		Dialogue = InDialogue;
		SequenceActor = InSequenceActor;

		AActor* NewLookAtActor = nullptr;
		AActor* NewFocusActor = nullptr;

		if (LookAtTrackingSettings.AvatarToTrack > EShotTrackingRule::STR_Disabled)
		{
			NewLookAtActor = (LookAtTrackingSettings.AvatarToTrack == EShotTrackingRule::STR_Speaker ? Speaker : Listener);

			if (LookAtTrackingSettings.AvatarToTrack == EShotTrackingRule::STR_Custom)
			{
				NewLookAtActor = Dialogue->GetAvatar(LookAtTrackingSettings.TrackedAvatarCustomID);
			}
		}

		if (FocusTrackingSettings.AvatarToTrack > EShotTrackingRule::STR_Disabled)
		{
			NewFocusActor = (FocusTrackingSettings.AvatarToTrack == EShotTrackingRule::STR_Speaker ? Speaker : Listener);

			if (FocusTrackingSettings.AvatarToTrack == EShotTrackingRule::STR_Custom)
			{
				NewFocusActor = Dialogue->GetAvatar(FocusTrackingSettings.TrackedAvatarCustomID);
			}
		}

		//We'd almost always want to use the speaker as the anchor, except for select shots like Over-The-Shoulder where we'd anchor to listeners shoulder 
		AActor* NewAnchorActor = nullptr;
		
		if (AnchorOriginRule >= EAnchorOriginRule::AOR_Speaker)
		{
			NewAnchorActor = (AnchorOriginRule == EAnchorOriginRule::AOR_Speaker ? Speaker : Listener);

			if (AnchorOriginRule == EAnchorOriginRule::AOR_Custom)
			{
				NewAnchorActor = Dialogue->GetAvatar(AnchorAvatarCustomID);
			}
		}


		//If this sequence is not supposed to restart, check if its already playing with the same actors.
		if (!bShouldRestart)
		{
			if (UNarrativeDialogueSequence* CurrentSequence = Dialogue->GetCurrentDialogueSequence())
			{
				//If the classes and shots are the same, with the same tracked and anchor actors we'll check if we can restart 
				if (CurrentSequence->GetClass() == GetClass() && CurrentSequence->SequenceAssets == SequenceAssets)
				{
					if (CurrentSequence->LookAtActor == NewLookAtActor && CurrentSequence->AnchorActor == NewAnchorActor)
					{
						return;
					}
				}
			}
		}

		InSequenceActor->SequencePlayer->Stop();

		AnchorActor = NewAnchorActor;
		LookAtActor = NewLookAtActor;
		FocusActor = NewFocusActor;

		if (AnchorOriginRule >= EAnchorOriginRule::AOR_Speaker && !IsValid(AnchorActor))
		{
			UE_LOG(LogTemp, Warning, TEXT("Shot %s is selected to anchor to speaker, but speaker could not be found. Aborting shot."), *GetNameSafe(this));
			return;
		}

		if (LookAtTrackingSettings.AvatarToTrack > EShotTrackingRule::STR_Disabled && !IsValid(LookAtActor))
		{
			UE_LOG(LogTemp, Warning, TEXT("Shot is supposed to track an actor, but TrackedActor was null. Aborting shot."));
		}

		if (FocusTrackingSettings.AvatarToTrack > EShotTrackingRule::STR_Disabled && !IsValid(FocusActor))
		{
			UE_LOG(LogTemp, Warning, TEXT("Shot is supposed to focus an actor, but TrackedActor was null. Aborting shot."));
		}

		PlaySequence();
	}
}

void UNarrativeDialogueSequence::EndSequence()
{	
	//We'll uncache all these to avoid any garbage collection issues, not sure if this is needed but wont hurt 
	//Speaker = nullptr;
	//Listener = nullptr;
	//AnchorActor = nullptr;
	//LookAtActor = nullptr;
	//Dialogue = nullptr;
	//SequenceActor = nullptr;
	//Cinecam = nullptr;
}

void UNarrativeDialogueSequence::PlaySequence_Implementation()
{
	if (SequenceAssets.Num())
	{
		int32 SequenceIdx = FMath::RandRange(0, SequenceAssets.Num() - 1);
		ULevelSequence* SelectedSequence = SequenceAssets[SequenceIdx];

		if (SelectedSequence)
		{
			SequenceActor->PlaybackSettings = PlaybackSettings;
			SequenceActor->SetSequence(SelectedSequence);

			if (SequenceActor->SequencePlayer)
			{
				SequenceActor->bOverrideInstanceData = AnchorOriginRule != EAnchorOriginRule::AOR_Disabled;

				if (UDefaultLevelSequenceInstanceData* InstanceData = Cast<UDefaultLevelSequenceInstanceData>(SequenceActor->DefaultInstanceData))
				{
					InstanceData->TransformOrigin = AnchorOriginRule != EAnchorOriginRule::AOR_Disabled ? GetShotAnchorTransform() : FTransform();
				}

				SequenceActor->SequencePlayer->Play();

				//Go in, and tell the cinecam to focus/track the speaker 
				for (auto& BoundObject : SequenceActor->SequencePlayer->GetBoundObjects(SequenceActor->FindNamedBinding(NAME_CinecamTag)))
				{
					Cinecam = Cast<ACineCameraActor>(BoundObject);

					if (Cinecam)
					{
						if (UCineCameraComponent* CinecamComp = Cinecam->GetCineCameraComponent())
						{
							CinecamComp->CropSettings = CropSettings;

							if (LookAtTrackingSettings.AvatarToTrack > EShotTrackingRule::STR_Disabled && LookAtActor)
							{
								const FVector Offset = LookAtActor->GetActorTransform().InverseTransformPosition(Dialogue->GetSpeakerHeadLocation(LookAtActor));
								//Focus the cinecam on the speaker 
								Cinecam->LookatTrackingSettings.bEnableLookAtTracking = true;
								Cinecam->LookatTrackingSettings.ActorToTrack = LookAtActor;
								Cinecam->LookatTrackingSettings.RelativeOffset = Offset + LookAtTrackingSettings.TrackBoneNudge;

								//Tracked shots should still offset themselves if using 180d rule
								if (bUse180DegreeRule)
								{
									Cinecam->LookatTrackingSettings.RelativeOffset += FVector(0.f, (Speaker == Dialogue->GetPlayerAvatar() ? UnitsY180DegreeRule : -UnitsY180DegreeRule), 0.f);
								}
							}

							if (FocusTrackingSettings.AvatarToTrack > EShotTrackingRule::STR_Disabled && FocusActor)
							{
								const FVector Offset = FocusActor->GetActorTransform().InverseTransformPosition(Dialogue->GetSpeakerHeadLocation(FocusActor));

								//Make the spawned cinecam track the head of the speaker
								CinecamComp->FocusSettings.FocusMethod = ECameraFocusMethod::Tracking;
								CinecamComp->FocusSettings.TrackingFocusSettings.ActorToTrack = FocusActor;
								CinecamComp->FocusSettings.TrackingFocusSettings.RelativeOffset = Offset + FocusTrackingSettings.TrackBoneNudge;
								CinecamComp->FocusSettings.TrackingFocusSettings.bDrawDebugTrackingFocusPoint = bDrawDebugFocusPoint;
							}
						}
					}
				}
			}
		}
	}
}

FTransform UNarrativeDialogueSequence::GetShotAnchorTransform_Implementation()
{
	FRotator OriginRot = FRotator::ZeroRotator;
	FVector OriginLoc = FVector::ZeroVector;
	
	if (AnchorOriginRule >= EAnchorOriginRule::AOR_Speaker)
	{
		OriginLoc = Dialogue->GetSpeakerHeadLocation(AnchorActor);
	}
	else if (AnchorOriginRule == EAnchorOriginRule::AOR_ConversationCenter)
	{
		OriginLoc = Dialogue->GetConversationCenterPoint();
	}

	if (AnchorRotationRule == EAnchorRotationRule::ARR_AnchorActorForwardVector)
	{
		if (AnchorActor)
		{
			OriginRot = AnchorActor->GetActorForwardVector().Rotation();
		}
	}
	else if (AnchorRotationRule == EAnchorRotationRule::ARR_Conversation)
	{
		OriginRot = (Dialogue->GetSpeakerHeadLocation(Speaker) - Dialogue->GetSpeakerHeadLocation(Listener)).Rotation();
	}

	FVector Scale = FVector::OneVector;

	if (AnchorActor)
	{
		//If using the 180d rule, we need to rotate the shot in a different direction based on who is talking
		//If tracking is enabled, don't do this as the tracking will apply the 180d offset later on 
		if (bUse180DegreeRule)
		{
			OriginLoc += (AnchorActor->GetActorRightVector() * (Speaker == Dialogue->GetPlayerAvatar() ? UnitsY180DegreeRule : -UnitsY180DegreeRule));
			OriginRot += FRotator(0.f, Speaker == Dialogue->GetPlayerAvatar() ? -DegreesYaw180DegreeRule : DegreesYaw180DegreeRule, 0.f);
		}

		const FVector AnchorNudgeUp = AnchorOriginNudge.Z * AnchorActor->GetActorUpVector();
		const FVector AnchorNudgeRight = AnchorOriginNudge.Y * AnchorActor->GetActorRightVector();
		const FVector AnchorNudgeFwd = AnchorOriginNudge.X * AnchorActor->GetActorForwardVector();
		OriginLoc += (AnchorNudgeUp + AnchorNudgeRight + AnchorNudgeFwd);
	}

	return FTransform(OriginRot, OriginLoc, Scale);
}

FText UNarrativeDialogueSequence::GetGraphDisplayText_Implementation()
{
	return FriendlyShotName.IsEmptyOrWhitespace() ? FText::FromString(GetName()) : FriendlyShotName;
}
