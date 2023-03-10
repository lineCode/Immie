// Fill out your copyright notice in the Description page of Project Settings.


#include "ImmieCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <Immie/Game/Global/Managers/TypeDataManager.h>
#include <Immie/Game/Global/Managers/AbilityDataManager.h>
#include <Immie/Game/Global/Managers/SpecieDataManager.h>
#include "ImmieObject.h"
#include <Kismet/GameplayStatics.h>
#include <Immie/Battle/BattleInstance/BattleInstance.h>
#include <Immie/Battle/Managers/BattleTypeManager.h>
#include <Immie/Battle/Managers/BattleAbilityManager.h>
#include <Immie/Battle/Managers/BattleSpecieManager.h>
#include <Immie/Battle/Team/BattleTeam.h>
#include <Immie/Ability/Abilities/Ability.h>
#include <Immie/Battle/Components/DamageComponent.h>
#include <Immie/Immies/SpecieDataObject.h>
#include <Kismet/KismetMathLibrary.h>
#include <Immie/Movement/ImmieMovementComponent.h>

AImmieCharacter::AImmieCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UImmieMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	bNetLoadOnClient = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	bUseControllerRotationPitch = true;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 900.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 1200.f; //I like this
	GetCharacterMovement()->AirControl = 0.6; //I like this
	GetCharacterMovement()->GravityScale = 3.f; //I like this

	GetCharacterMovement()->MaxWalkSpeed = 900.f;
	GetCharacterMovement()->MaxFlySpeed = 500.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FollowCamera->SetupAttachment(RootComponent);
	FollowCamera->bUsePawnControlRotation = true;

	ImmieObject = nullptr;
	bEnabled = true;
	Abilities.Reserve(MAX_ABILITY_COUNT);
	Team = nullptr;
}

void AImmieCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AImmieCharacter::PressAbility(int Index)
{
	iLog("Pressed ability with index " + FString::FromInt(Index));
	UAbility* Ability = Abilities[Index];
	Ability->InputPress();
}

void AImmieCharacter::ReleaseAbility(int Index)
{
	iLog("Released ability with index " + FString::FromInt(Index));
	UAbility* Ability = Abilities[Index];
	Ability->InputRelease();
}

void AImmieCharacter::ImmieYawInput(float AxisValue)
{
	AddControllerYawInput(AxisValue * 0.5);
}

void AImmieCharacter::ImmiePitchInput(float AxisValue)
{
	AddControllerPitchInput(AxisValue * 0.5);
}

void AImmieCharacter::AddForwardMovement(float ScaleValue)
{
	AddMovementInput(UKismetMathLibrary::GetForwardVector(FRotator(0, GetControlRotation().Yaw, 0)), ScaleValue, false);
}

void AImmieCharacter::AddRightMovement(float ScaleValue)
{
	AddMovementInput(UKismetMathLibrary::GetRightVector(FRotator(0, GetControlRotation().Yaw, 0)), ScaleValue, false);
}

void AImmieCharacter::Jump()
{
	Super::Jump();
}

void AImmieCharacter::StopJumping()
{
	Super::StopJumping();
}

void AImmieCharacter::Crouch()
{
}

void AImmieCharacter::StopCrouching()
{
}

void AImmieCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AImmieCharacter::AuthorityBattleTick_Implementation(float DeltaTime)
{
	AuthorityBattleTickComponents(DeltaTime);
	if (!IsRunningDedicatedServer()) {
		IBattleActor::Execute_UpdateVisuals(this);
	}
}

void AImmieCharacter::ClientBattleTick_Implementation(float DeltaTime)
{
	ClientBattleTickComponents(DeltaTime);
	IBattleActor::Execute_UpdateVisuals(this);
}

void AImmieCharacter::AuthorityBattleTickComponents(float DeltaTime)
{
	for (int i = 0; i < Abilities.Num(); i++) {
		Abilities[i]->AuthorityBattleTick(DeltaTime);
	}
	DamageComponent->AuthorityBattleTick(DeltaTime);
}

void AImmieCharacter::ClientBattleTickComponents(float DeltaTime)
{
	for (int i = 0; i < Abilities.Num(); i++) {
		Abilities[i]->ClientBattleTick(DeltaTime);
	}
	DamageComponent->ClientBattleTick(DeltaTime);
}

void AImmieCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);

	/* Rotation. */
	PlayerInputComponent->BindAxis("Turn", this, &AImmieCharacter::ImmieYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &AImmieCharacter::ImmiePitchInput);

	/* WASD. */
	PlayerInputComponent->BindAxis("MoveForward", this, &AImmieCharacter::AddForwardMovement);
	PlayerInputComponent->BindAxis("MoveRight", this, &AImmieCharacter::AddRightMovement);

	/* Jump. */
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AImmieCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AImmieCharacter::StopJumping);

	/* Crouch. */
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AImmieCharacter::Crouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &AImmieCharacter::StopCrouching);

	/* Immie ability inputs. */
	PlayerInputComponent->BindAction("InputAbility0", IE_Pressed, this, &AImmieCharacter::PressAbility0);
	PlayerInputComponent->BindAction("InputAbility1", IE_Pressed, this, &AImmieCharacter::PressAbility1);
	PlayerInputComponent->BindAction("InputAbility2", IE_Pressed, this, &AImmieCharacter::PressAbility2);
	PlayerInputComponent->BindAction("InputAbility3", IE_Pressed, this, &AImmieCharacter::PressAbility3);

	PlayerInputComponent->BindAction("InputAbility0", IE_Released, this, &AImmieCharacter::ReleaseAbility0);
	PlayerInputComponent->BindAction("InputAbility1", IE_Released, this, &AImmieCharacter::ReleaseAbility1);
	PlayerInputComponent->BindAction("InputAbility2", IE_Released, this, &AImmieCharacter::ReleaseAbility2);
	PlayerInputComponent->BindAction("InputAbility3", IE_Released, this, &AImmieCharacter::ReleaseAbility3);
}

void AImmieCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
}

void AImmieCharacter::UnPossessed()
{
	Super::UnPossessed();
}

AImmieCharacter* AImmieCharacter::NewImmieCharacter(AActor* _Owner, const FTransform& Transform, UImmie* _ImmieObject, bool EnabledOnSpawn)
{
	UClass* ImmieCharacterClass = GetSpecieDataManager()->GetImmieCharacterClass(_ImmieObject->GetSpecieId());
	AImmieCharacter* SpawnedImmie = 
		_Owner->GetWorld()->SpawnActorDeferred<AImmieCharacter>(ImmieCharacterClass, Transform, _Owner, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

	check(IsValid(SpawnedImmie));
	SpawnedImmie->ImmieObject = _ImmieObject;

	if (!EnabledOnSpawn) {
		SpawnedImmie->SetImmieEnabled(false);
	}

	UGameplayStatics::FinishSpawningActor(SpawnedImmie, Transform);
	return SpawnedImmie;
}

void AImmieCharacter::PossessForBattle(AController* NewController)
{
	NewController->Possess(this);
	
	const bool IsPlayerController = NewController->IsPlayerController();
	if (IsPlayerController) {
		AImmiePlayerController* ImmiePlayerController = Cast<AImmiePlayerController>(NewController);
		ClientPossessedByPlayerController(ImmiePlayerController);
	}
}

void AImmieCharacter::ClientPossessedByPlayerController_Implementation(AImmiePlayerController* PlayerController)
{
	BP_ClientPossessedByPlayerController(PlayerController);
}

void AImmieCharacter::UnPossessForBattle()
{
	if (GetController() == nullptr) {
		return;
	}

	const bool IsPlayerController = GetController()->IsPlayerController();
	if (IsPlayerController) {
		AImmiePlayerController* ImmiePlayerController = Cast<AImmiePlayerController>(GetController());
		ClientPossessedByPlayerController(ImmiePlayerController);
	}
	GetController()->UnPossess();
}

void AImmieCharacter::ClientUnPossessedByPlayerController_Implementation(AImmiePlayerController* PlayerController)
{
	BP_ClientUnPossessedByPlayerController(PlayerController);
}

void AImmieCharacter::InitializeForBattle(ABattleTeam* OwningTeam, int SlotOnTeam)
{
	Team = OwningTeam;
	TeamSlot = SlotOnTeam;

	const int SpecieId = ImmieObject->GetSpecieId();
	USpecieDataObject* SpecieDataObject = GetBattleInstance()->GetBattleSpecieManager()->GetSpecieDataObject(SpecieId);

	const int SpecieTypeBitmask = SpecieDataObject->GetTypeBitmask();
	Type = GetBattleInstance()->GetBattleTypeManager()->GetTypes(SpecieTypeBitmask);

	Level = UFormula::LevelFromXp(ImmieObject->GetXp());

	const FBaseStats BaseStats = SpecieDataObject->GetBaseStats();
	InitialStats = ImmieObject->GetStats(true, BaseStats.Health, BaseStats.Attack, BaseStats.Defense, BaseStats.Speed);

	ActiveStats.Health = ImmieObject->GetHealth();
	ActiveStats.Attack = InitialStats.Attack;
	ActiveStats.Defense = InitialStats.Defense;
	ActiveStats.Speed = InitialStats.Speed;

	DamageComponent = UDamageComponent::NewDamageComponent(this);

	const TArray<int> AbilityIds = ImmieObject->GetAbilityIds();
	const int AbilityCount = AbilityIds.Num();
	for (int i = 0; i < AbilityCount; i++) {
		UAbility* BattleAbility = UAbility::NewAbility(this, AbilityIds[i]);
		BattleAbility->InitializeForBattle();
		Abilities.Add(BattleAbility);
	}

	BP_InitializeForBattle(SpecieDataObject);
}

void AImmieCharacter::SyncClientSubobjects()
{
	FString ImmieObjectString = ImmieObject->SaveJsonData().ToString();
	SetClientSubobjects(ImmieObjectString, Type, Abilities, Team, DamageComponent);
	BP_SyncClientSubobjects();
}

void AImmieCharacter::SyncClientBattleData()
{
	SetClientBattleData(
		TeamSlot,
		Level,
		InitialStats,
		ActiveStats
	);

	for (int i = 0; i < Abilities.Num(); i++) {
		Abilities[i]->SyncToClients();
	}

	BP_SyncClientBattleData();
}

void AImmieCharacter::SetClientBattleData_Implementation(int _TeamSlot, uint8 _Level, FBattleStats _InitialStats, FBattleStats _ActiveStats)
{
	if (Team->HasAuthority()) {
		return;
	}

	TeamSlot = _TeamSlot;
	Level = _Level;
	InitialStats = _InitialStats;
	ActiveStats = _ActiveStats;
}

void AImmieCharacter::SetClientSubobjects_Implementation(const FString& ImmieObjectString, const TArray<UImmieType*>& TypesObjects, const TArray<UAbility*>& AbilityObjects, ABattleTeam* BattleTeamObject, UDamageComponent* DamageComponentObject)
{
	if (BattleTeamObject->HasAuthority()) {
		return;
	}

	Type = TypesObjects;
	Abilities = AbilityObjects;
	Team = BattleTeamObject;
	DamageComponent = DamageComponentObject;

	FJsonObjectBP ImmieJson;
	if (FJsonObjectBP::LoadJsonString(ImmieObjectString, ImmieJson)) {
		FName ParsedSpecieName = FName(ImmieJson.GetStringField("Specie"));
		const int SpecieId = GetSpecieDataManager()->GetSpecieId(ParsedSpecieName);
		ImmieObject = UImmie::NewImmieObject(this, SpecieId);
		ImmieObject->LoadJsonData(SpecieId, ImmieJson);
	}
}

bool AImmieCharacter::AllClientBattleSubobjectsValid()
{
	bool IsValidSubobjects = true;

	if (!IsValid(ImmieObject))
		return false;

	if (Type.Num() == 0)
		return false;

	if (Abilities.Num() == 0)
		return false;

	for (int i = 0; i < Abilities.Num(); i++) {
		if (!IsValid(Abilities[i])) {
			return false;
		}
	}

	if (!IsValid(Team))
		return false;

	if (!IsValid(DamageComponent))
		return false;
	
	bool IsValidBlueprintSubobjects = BP_AllClientBattleSubobjectsValid();

	return IsValidSubobjects && IsValidBlueprintSubobjects;
}

int AImmieCharacter::GetSpecieId() const
{
	return ImmieObject->GetSpecieId();
}

FName AImmieCharacter::GetSpecieName() const
{
	return GetSpecieDataManager()->GetSpecieName(GetSpecieId());
}

bool AImmieCharacter::BP_AllClientBattleSubobjectsValid_Implementation()
{
	return true;
}

ABattleInstance* AImmieCharacter::GetBattleInstance() const
{
	return Team->GetBattleInstance();
}

UDamageComponent* AImmieCharacter::GetDamageComponent_Implementation() const
{
	return DamageComponent;
}

float AImmieCharacter::TotalHealingFromAbility_Implementation(const FAbilityInstigatorDamage& AbilityHealing) const
{
	return 0.0f;
}

float AImmieCharacter::TotalDamageFromAbility_Implementation(const FAbilityInstigatorDamage& AbilityDamage) const
{
	return 0.0f;
}

void AImmieCharacter::SetImmieEnabled_Implementation(bool NewEnabled)
{
	SetActorTickEnabled(NewEnabled);
	SetActorHiddenInGame(!NewEnabled);
	SetActorEnableCollision(NewEnabled);
	bEnabled = NewEnabled;
}

