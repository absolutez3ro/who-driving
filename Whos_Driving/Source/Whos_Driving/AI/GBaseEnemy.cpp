// Fill out your copyright notice in the Description page of Project Settings.


#include "GBaseEnemy.h"
#include "GEnemyBaseDataAsset.h"

// Sets default values
AGBaseEnemy::AGBaseEnemy()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AGBaseEnemy::BeginPlay()
{
	Super::BeginPlay();
	maxHp = baseData->maxHp;
	currentHp = maxHp;
	scrapPrice = baseData->scrapPrice;
	USkeletalMesh* targetMesh = baseData->mesh;
	if (targetMesh)
	{
		// For ACharacter, use GetMesh(). For AActor, use FindComponentByClass<USkeletalMeshComponent>()
		USkeletalMeshComponent* meshComponent = FindComponentByClass<USkeletalMeshComponent>();

		if (!meshComponent)
		{
			UE_LOG(LogTemp, Error, TEXT("No skeletal mesh found for enemy"));
			meshComponent = NewObject<USkeletalMeshComponent>(this, USkeletalMeshComponent::StaticClass());
			meshComponent->RegisterComponent();
			meshComponent->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
		}
		meshComponent->SetSkeletalMesh(targetMesh);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load Skeletal Mesh from Data Asset on %s"), *GetName());
	}
}

// Called every frame
void AGBaseEnemy::Tick(float deltaTime)
{
	Super::Tick(deltaTime);

}

void AGBaseEnemy::HandleDeath()
{
	if (!HasAuthority())
	{
		return;
	}
	Destroy(true, false);
}

float AGBaseEnemy::TakeDamage(float damageAmount, FDamageEvent const& damageEvent, AController* eventInstigator, AActor* damageCauser)
{
	if (!HasAuthority())
	{
		return 0.0f;
	}
	if (damageAmount > 0)
	{
		currentHp -= damageAmount;
		UE_LOG(LogTemp, Log, TEXT("damage: %f"), damageAmount);

		UE_LOG(LogTemp, Log, TEXT("hp left: %f / %f"), currentHp, maxHp);
		if (currentHp <= 0)
		{
			onDeath.Broadcast(scrapPrice);
			HandleDeath();
		}
	}
	return damageAmount;
}
