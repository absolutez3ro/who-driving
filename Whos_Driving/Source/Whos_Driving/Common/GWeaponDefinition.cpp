#include "GWeaponDefinition.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/DataValidation.h"

#if WITH_EDITOR
EDataValidationResult UGWeaponDefinition::IsDataValid(FDataValidationContext& context) const
{
	Super::IsDataValid(context);
	const auto addError = [&context](const FString& message)
	{
		context.AddError(FText::FromString(message));
	};
	if (!Mesh)
	{
		addError(TEXT("Mesh is not set."));
	}
	else
	{
		TArray<FName> requiredSockets = {MuzzleSocket, ShellSocket, MagazineSocket};
		for (const FGWeaponAttachment& attachment : Attachments)
		{
			if (attachment.Mesh)
			{
				requiredSockets.Add(attachment.Socket);
			}
		}
		for (const FName socket : requiredSockets)
		{
			if (!Mesh->FindSocket(socket))
			{
				addError(FString::Printf(TEXT("Mesh %s has no socket %s."), *Mesh->GetName(), *socket.ToString()));
			}
		}
	}
	if (FireModes.IsEmpty())
	{
		addError(TEXT("FireModes is empty."));
	}
	else if (FireModes.Contains(EGWeaponFireMode::Burst) && BurstSize < 1)
	{
		addError(TEXT("FireModes contains Burst, but BurstSize is less than 1."));
	}
	if (!IdlePose)
	{
		addError(TEXT("IdlePose is not set."));
	}
	return context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
