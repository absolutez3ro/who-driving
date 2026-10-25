#include "G_Combat_Editor_Library.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "G_Vehicle_Combat_Anim_Instance.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimBlueprintGeneratedClass.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/AnimBlueprintFactory.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_SequenceEvaluator.h"
#include "AnimGraphNode_LayeredBoneBlend.h"
#include "AnimGraphNode_LocalToComponentSpace.h"
#include "AnimGraphNode_ComponentToLocalSpace.h"
#include "AnimGraphNode_ModifyBone.h"
#include "AnimGraphNode_TwoBoneIK.h"
#include "K2Node_VariableGet.h"
#include "Modules/ModuleManager.h"
#include "Misc/PackageName.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, WhosDrivingCombatEditor)

bool UGCombatEditorLibrary::InjectPlayerKey(APlayerController* controller, FName keyName, bool bPressed)
{
	const FKey key(keyName);
	return IsValid(controller) && controller->IsLocalController() && key.IsValid()
		&& controller->InputKey(FInputKeyEventArgs::CreateSimulated(key, bPressed ? IE_Pressed : IE_Released, bPressed ? 1.0 : 0.0));
}

bool UGCombatEditorLibrary::CopyM8Packages(const TArray<FString>& sourcePackages)
{
	TMap<FString, FString> copies;
	for (const FString& source : sourcePackages)
	{
		if (!source.StartsWith(TEXT("/Game/")) || source.StartsWith(TEXT("/Game/M8Shooter/")))
		{
			return false;
		}
		const FString destination = TEXT("/Game/M8Shooter/") + source.RightChop(6);
		if (FPackageName::DoesPackageExist(destination))
		{
			return false;
		}
		copies.Add(source, destination);
	}
	return FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"))
		.Get()
		.AdvancedCopyPackages(copies, false, false);
}

namespace
{
template <typename T> T* AddNode(UEdGraph* graph, int32 x, int32 y)
{
	T* node = NewObject<T>(graph);
	graph->AddNode(node, false, false);
	node->CreateNewGuid();
	node->NodePosX = x;
	node->NodePosY = y;
	node->PostPlacedNewNode();
	node->AllocateDefaultPins();
	return node;
}
UEdGraphPin* Output(UEdGraphNode* node)
{
	for (UEdGraphPin* pin : node->Pins)
	{
		if (pin->Direction == EGPD_Output)
		{
			return pin;
		}
	}
	return nullptr;
}
void Connect(UEdGraph* graph, UEdGraphPin* output, UEdGraphPin* input)
{
	check(output && input);
	check(graph->GetSchema()->TryCreateConnection(output, input));
}
void Bind(UEdGraph* graph, UAnimGraphNode_Base* node, FName pinName, FName variableName)
{
	for (int32 index = 0; index < node->ShowPinForProperties.Num(); ++index)
	{
		if (node->ShowPinForProperties[index].PropertyName == pinName)
		{
			node->SetPinVisibility(true, index);
			break;
		}
	}
	UK2Node_VariableGet* variable = NewObject<UK2Node_VariableGet>(graph);
	variable->VariableReference.SetSelfMember(variableName);
	graph->AddNode(variable, false, false);
	variable->CreateNewGuid();
	variable->NodePosX = node->NodePosX - 250;
	variable->NodePosY = node->NodePosY + node->Pins.Num() * 30;
	variable->AllocateDefaultPins();
	Connect(graph, Output(variable), node->FindPin(pinName));
}
} // namespace

UAnimBlueprint* UGCombatEditorLibrary::CreateCombatAnimBlueprint(const FString& packageName, USkeleton* skeleton,
																 UAnimSequence* basePose)
{
	if (!skeleton || !basePose || !packageName.StartsWith(TEXT("/Game/M8Shooter/")) ||
		FPackageName::DoesPackageExist(packageName))
	{
		return nullptr;
	}
	UPackage* package = CreatePackage(*packageName);
	UAnimBlueprintFactory* factory = NewObject<UAnimBlueprintFactory>();
	factory->ParentClass = UGVehicleCombatAnimInstance::StaticClass();
	factory->TargetSkeleton = skeleton;
	UAnimBlueprint* blueprint = Cast<UAnimBlueprint>(factory->FactoryCreateNew(
		UAnimBlueprint::StaticClass(), package, FName(*FPackageName::GetLongPackageAssetName(packageName)),
		RF_Public | RF_Standalone, nullptr, GWarn));
	if (!blueprint)
	{
		return nullptr;
	}
	TArray<UEdGraph*> graphs;
	blueprint->GetAllGraphs(graphs);
	UEdGraph* graph = nullptr;
	UAnimGraphNode_Root* root = nullptr;
	for (UEdGraph* candidate : graphs)
	{
		for (UEdGraphNode* node : candidate->Nodes)
		{
			if (UAnimGraphNode_Root* candidateRoot = Cast<UAnimGraphNode_Root>(node))
			{
				graph = candidate;
				root = candidateRoot;
				break;
			}
		}
	}
	if (!graph || !root)
	{
		return nullptr;
	}
	UAnimGraphNode_SequenceEvaluator* base = AddNode<UAnimGraphNode_SequenceEvaluator>(graph, -2600, -300);
	base->Node.SetSequence(basePose);
	base->ReconstructNode();
	UAnimGraphNode_SequenceEvaluator* upper = AddNode<UAnimGraphNode_SequenceEvaluator>(graph, -2600, 100);
	upper->Node.SetSequence(basePose);
	upper->Node.SetTeleportToExplicitTime(false);
	upper->ReconstructNode();
	Bind(graph, upper, TEXT("Sequence"), TEXT("weaponPose"));
	Bind(graph, upper, TEXT("ExplicitTime"), TEXT("poseTime"));
	UAnimGraphNode_LayeredBoneBlend* layer = AddNode<UAnimGraphNode_LayeredBoneBlend>(graph, -2000, 0);
	if (layer->Node.BlendPoses.IsEmpty())
	{
		layer->AddPinToBlendByFilter();
	}
	layer->Node.LayerSetup.SetNum(1);
	FBranchFilter filter;
	filter.BoneName = TEXT("spine_01");
	filter.BlendDepth = 1;
	layer->Node.LayerSetup[0].BranchFilters.Add(filter);
	layer->Node.bMeshSpaceRotationBlend = true;
	layer->ReconstructNode();
	Connect(graph, Output(base), layer->FindPin(TEXT("BasePose")));
	Connect(graph, Output(upper), layer->FindPin(TEXT("BlendPoses_0")));
	Bind(graph, layer, TEXT("BlendWeights_0"), TEXT("weaponAlpha"));
	UAnimGraphNode_LocalToComponentSpace* component = AddNode<UAnimGraphNode_LocalToComponentSpace>(graph, -1600, 0);
	Connect(graph, Output(layer), component->FindPin(TEXT("LocalPose")));
	UEdGraphPin* previous = Output(component);
	int32 x = -1200;
	auto modify = [&](FName bone, FName translation, FName rotation)
	{
		UAnimGraphNode_ModifyBone* node = AddNode<UAnimGraphNode_ModifyBone>(graph, x, 0);
		node->Node.BoneToModify.BoneName = bone;
		node->Node.TranslationMode = translation.IsNone() ? BMM_Ignore : BMM_Additive;
		node->Node.RotationMode = rotation.IsNone() ? BMM_Ignore : BMM_Additive;
		node->Node.TranslationSpace = BCS_ComponentSpace;
		node->Node.RotationSpace = BCS_ComponentSpace;
		node->ReconstructNode();
		Connect(graph, previous, node->FindPin(TEXT("ComponentPose")));
		Bind(graph, node, TEXT("Alpha"), TEXT("seatAlpha"));
		if (!translation.IsNone())
		{
			Bind(graph, node, TEXT("Translation"), translation);
		}
		if (!rotation.IsNone())
		{
			Bind(graph, node, TEXT("Rotation"), rotation);
		}
		previous = Output(node);
		x += 450;
	};
	modify(TEXT("pelvis"), TEXT("pelvisOffset"), NAME_None);
	modify(TEXT("thigh_l"), NAME_None, TEXT("thighRotation"));
	modify(TEXT("thigh_r"), NAME_None, TEXT("thighRotation"));
	modify(TEXT("calf_l"), NAME_None, TEXT("calfRotation"));
	modify(TEXT("calf_r"), NAME_None, TEXT("calfRotation"));
	modify(TEXT("spine_01"), TEXT("spineOffset"), TEXT("spineRotation"));
	modify(TEXT("spine_02"), TEXT("spineOffset"), TEXT("spineRotation"));
	modify(TEXT("spine_03"), TEXT("spineOffset"), TEXT("spineRotation"));
	for (int32 side = 0; side < 2; ++side)
	{
		UAnimGraphNode_TwoBoneIK* ik = AddNode<UAnimGraphNode_TwoBoneIK>(graph, x, 0);
		ik->Node.IKBone.BoneName = side == 0 ? TEXT("hand_r") : TEXT("hand_l");
		ik->Node.EffectorLocationSpace = BCS_ComponentSpace;
		ik->Node.JointTargetLocationSpace = BCS_ComponentSpace;
		ik->Node.bAllowStretching = false;
		ik->Node.bTakeRotationFromEffectorSpace = false;
		ik->Node.bMaintainEffectorRelRot = false;
		ik->ReconstructNode();
		Connect(graph, previous, ik->FindPin(TEXT("ComponentPose")));
		Bind(graph, ik, TEXT("Alpha"), TEXT("handIkAlpha"));
		Bind(graph, ik, TEXT("EffectorLocation"), side == 0 ? TEXT("rightHandLocation") : TEXT("leftHandLocation"));
		Bind(graph, ik, TEXT("JointTargetLocation"),
			 side == 0 ? TEXT("rightElbowLocation") : TEXT("leftElbowLocation"));
		previous = Output(ik);
		x += 450;
	}
	modify(TEXT("hand_r"), NAME_None, TEXT("weaponRotation"));
	UAnimGraphNode_ComponentToLocalSpace* local = AddNode<UAnimGraphNode_ComponentToLocalSpace>(graph, x, 0);
	Connect(graph, previous, local->FindPin(TEXT("ComponentPose")));
	Connect(graph, Output(local), root->FindPin(TEXT("Result")));
	root->NodePosX = x + 300;
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(blueprint);
	FKismetEditorUtilities::CompileBlueprint(blueprint);
	if (UGVehicleCombatAnimInstance* defaults =
			Cast<UGVehicleCombatAnimInstance>(blueprint->GeneratedClass->GetDefaultObject()))
	{
		defaults->basePose = basePose;
	}
	FAssetRegistryModule::AssetCreated(blueprint);
	package->MarkPackageDirty();
	return blueprint;
}

bool UGCombatEditorLibrary::AddMeshSocket(USkeletalMesh* mesh, FName socketName, FName boneName, FTransform offset)
{
	if (!mesh || mesh->GetRefSkeleton().FindBoneIndex(boneName) == INDEX_NONE)
	{
		return false;
	}
	for (USkeletalMeshSocket* socket : mesh->GetMeshOnlySocketList())
	{
		if (socket && socket->SocketName == socketName)
		{
			mesh->Modify();
			socket->BoneName = boneName;
			socket->RelativeLocation = offset.GetLocation();
			socket->RelativeRotation = offset.Rotator();
			socket->RelativeScale = offset.GetScale3D();
			mesh->MarkPackageDirty();
			return true;
		}
	}
	mesh->Modify();
	USkeletalMeshSocket* socket = NewObject<USkeletalMeshSocket>(mesh);
	socket->SocketName = socketName;
	socket->BoneName = boneName;
	socket->RelativeLocation = offset.GetLocation();
	socket->RelativeRotation = offset.Rotator();
	socket->RelativeScale = offset.GetScale3D();
	mesh->GetMeshOnlySocketList().Add(socket);
	mesh->MarkPackageDirty();
	return true;
}

bool UGCombatEditorLibrary::AlignCombatHand(UAnimBlueprint* blueprint)
{
	if (!blueprint || !blueprint->GetPathName().StartsWith(TEXT("/Game/M8Shooter/")))
	{
		return false;
	}
	TArray<UEdGraph*> graphs;
	blueprint->GetAllGraphs(graphs);
	for (UEdGraph* graph : graphs)
	{
		for (UEdGraphNode* node : graph->Nodes)
		{
			if (const UAnimGraphNode_ModifyBone* existing = Cast<UAnimGraphNode_ModifyBone>(node))
			{
				if (existing->Node.BoneToModify.BoneName == TEXT("hand_r") &&
					existing->Node.RotationMode == BMM_Replace)
				{
					FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(blueprint);
					FKismetEditorUtilities::CompileBlueprint(blueprint);
					return true;
				}
			}
		}
		for (UEdGraphNode* node : graph->Nodes)
		{
			UAnimGraphNode_TwoBoneIK* ik = Cast<UAnimGraphNode_TwoBoneIK>(node);
			if (!ik || ik->Node.IKBone.BoneName != TEXT("hand_r"))
			{
				continue;
			}
			blueprint->Modify();
			UEdGraphPin* previous = Output(ik);
			const TArray<UEdGraphPin*> destinations = previous->LinkedTo;
			previous->BreakAllPinLinks();
			UAnimGraphNode_ModifyBone* rotation = AddNode<UAnimGraphNode_ModifyBone>(graph, ik->NodePosX + 200, 400);
			rotation->Node.BoneToModify.BoneName = TEXT("hand_r");
			rotation->Node.TranslationMode = BMM_Ignore;
			rotation->Node.RotationMode = BMM_Replace;
			rotation->Node.RotationSpace = BCS_ComponentSpace;
			rotation->ReconstructNode();
			Connect(graph, previous, rotation->FindPin(TEXT("ComponentPose")));
			Bind(graph, rotation, TEXT("Alpha"), TEXT("handIkAlpha"));
			Bind(graph, rotation, TEXT("Rotation"), TEXT("rightHandRotation"));
			for (UEdGraphPin* destination : destinations)
			{
				Connect(graph, Output(rotation), destination);
			}
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(blueprint);
			FKismetEditorUtilities::CompileBlueprint(blueprint);
			return blueprint->Status != BS_Error;
		}
	}
	return false;
}

bool UGCombatEditorLibrary::SetCombatArmStretch(UAnimBlueprint* blueprint, double maximumScale)
{
	if (!blueprint || !blueprint->GetPathName().StartsWith(TEXT("/Game/M8Shooter/")))
	{
		return false;
	}
	blueprint->Modify();
	TArray<UEdGraph*> graphs;
	blueprint->GetAllGraphs(graphs);
	for (UEdGraph* graph : graphs)
	{
		for (UEdGraphNode* node : graph->Nodes)
		{
			if (UAnimGraphNode_TwoBoneIK* ik = Cast<UAnimGraphNode_TwoBoneIK>(node))
			{
				ik->Modify();
				ik->Node.bAllowStretching = maximumScale > 1.0;
				ik->Node.StartStretchRatio = 1.0;
				ik->Node.MaxStretchScale = FMath::Clamp(maximumScale, 1.0, 2.0);
			}
		}
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(blueprint);
	FKismetEditorUtilities::CompileBlueprint(blueprint);
	return blueprint->Status != BS_Error;
}
