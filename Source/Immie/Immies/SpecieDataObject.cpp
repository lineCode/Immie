
// Fill out your copyright notice in the Description page of Project Settings.


#include "SpecieDataObject.h"
#include <Immie/ImmieCore.h>
#include <Immie/Game/Global/Managers/TypeDataManager.h>
#include <Immie/Game/Global/Managers/AbilityDataManager.h>

#define SPECIE_DO_JSON_FIELD_TYPES "Types"
#define SPECIE_DO_JSON_FIELD_BASE_HEALTH "BaseHealth"
#define SPECIE_DO_JSON_FIELD_BASE_ATTACK "BaseAttack"
#define SPECIE_DO_JSON_FIELD_BASE_DEFENSE "BaseDefense"
#define SPECIE_DO_JSON_FIELD_BASE_SPEED "BaseSpeed"
#define SPECIE_DO_JSON_FIELD_LEARNSET "Learnset"
#define SPECIE_DO_JSON_FIELD_LEARNSET_INITIAL "Initial"
#define SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_10 "Level10"
#define SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_20 "Level20"
#define SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_30 "Level30"
#define SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_40 "Level40"
#define SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_50 "Level50"
#define SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_60 "Level60"
#define SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_70 "Level70"
#define SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_80 "Level80"
#define SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_90 "Level90"
#define SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_100 "Level100"
#define SPECIE_DO_JSON_FIELD_ABILITY "Ability"
#define SPECIE_DO_JSON_FIELD_SPECIE_SPECIFIC "SpecieSpecific"

USpecieDataObject::USpecieDataObject()
{
    SpecieId = -1;
}

void USpecieDataObject::LoadClasses()
{
    if (SpecieName == "None") {
        iLog("Specie name is \"None\" for specie data object " + GetFName().ToString(), LogVerbosity_Error);
        return;
    }

    FString SpecieString = FName::NameToDisplayString(SpecieName.ToString(), false);

    const FString CharacterClassReferenceString = GetImmiesBlueprintFolder() + SpecieString + "/" + SpecieString + "Character_BP." + SpecieString + "Character_BP_C'";
    const FString ObjectClassReferenceString = GetImmiesBlueprintFolder() + SpecieString + "/" + SpecieString + "Object_BP." + SpecieString + "Object_BP_C'";

    UClass* SpecieObjectClass = Cast<UClass>(StaticLoadObject(UClass::StaticClass(), NULL, *ObjectClassReferenceString));
    UClass* SpecieCharacterClass = Cast<UClass>(StaticLoadObject(UClass::StaticClass(), NULL, *CharacterClassReferenceString));

    ObjectClass = SpecieObjectClass;
    CharacterClass = SpecieCharacterClass;
}

const FString& USpecieDataObject::GetImmiesBlueprintFolder()
{
    static const FString ImmiesBlueprintFolder = "/Script/Engine.Blueprint'/Game/Immies/";
    return ImmiesBlueprintFolder;
}

USpecieDataObject* USpecieDataObject::CreateSpecieDataObject(UObject* Outer, int _SpecieId, UClass* DataObjectClass)
{
    USpecieDataObject* SpecieDataObject = NewObject<USpecieDataObject>(Outer, DataObjectClass);
    SpecieDataObject->SpecieId = _SpecieId;
    SpecieDataObject->LoadClasses();
    return SpecieDataObject;
}

void USpecieDataObject::CheckClassesValid()
{
    iLog(IsValid(ObjectClass) ? "object class is valid" : "object class no valid :(");
    iLog(IsValid(CharacterClass) ? "character class is valid" : "character class no valid :(");
}

void USpecieDataObject::LoadSpecieJsonData(const FJsonObjectBP& Json, bool LoadJsonLearnsets)
{
    TypeBitmask = GetTypeDataManager()->GetTypeBitmaskFromJsonArray(Json.GetArrayField(SPECIE_DO_JSON_FIELD_TYPES));
    BaseStats.Health = Json.GetIntegerField(SPECIE_DO_JSON_FIELD_BASE_HEALTH);
    BaseStats.Attack = Json.GetIntegerField(SPECIE_DO_JSON_FIELD_BASE_ATTACK);
    BaseStats.Defense = Json.GetIntegerField(SPECIE_DO_JSON_FIELD_BASE_DEFENSE);
    BaseStats.Speed = Json.GetIntegerField(SPECIE_DO_JSON_FIELD_BASE_SPEED);

    if (LoadJsonLearnsets) {
        FJsonObjectBP AbilityLearnsets;
        if (Json.TryGetObjectField(SPECIE_DO_JSON_FIELD_LEARNSET, AbilityLearnsets)) {
            LoadLearnsets(AbilityLearnsets);
        }
    }
    

    FJsonObjectBP SpecieSpecific;
    if (Json.TryGetObjectField(SPECIE_DO_JSON_FIELD_SPECIE_SPECIFIC, SpecieSpecific)) {
        BP_LoadSpecieJsonData(SpecieSpecific);
    }
}

void USpecieDataObject::LoadLearnsets(const FJsonObjectBP& Json)
{
    Learnset.Initial = LoadAbilitySet(Json, SPECIE_DO_JSON_FIELD_LEARNSET_INITIAL);
    Learnset.Level10 = LoadAbilitySet(Json, SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_10);
    Learnset.Level20 = LoadAbilitySet(Json, SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_20);
    Learnset.Level30 = LoadAbilitySet(Json, SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_30);
    Learnset.Level40 = LoadAbilitySet(Json, SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_40);
    Learnset.Level50 = LoadAbilitySet(Json, SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_50);
    Learnset.Level60 = LoadAbilitySet(Json, SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_60);
    Learnset.Level70 = LoadAbilitySet(Json, SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_70);
    Learnset.Level80 = LoadAbilitySet(Json, SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_80);
    Learnset.Level90 = LoadAbilitySet(Json, SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_90);
    Learnset.Level100 = LoadAbilitySet(Json, SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_100);
}

TArray<int> USpecieDataObject::LoadAbilitySet(const FJsonObjectBP& Json, const FString& ArrayName)
{
    TArray<FString> Parsed = Json.GetArrayField(ArrayName).GetStringArray("Ability");
    const int ElementCount = Parsed.Num();

    TArray<int> Abilities;
    Abilities.Reserve(ElementCount);
    for (int i = 0; i < ElementCount; i++) {
        const int AbilityId = GetAbilityDataManager()->GetAbilityId(FName(Parsed[i]));

        if (GetAbilityDataManager()->IsValidAbility(AbilityId)) {
            Abilities.Add(AbilityId);
        }
        else {
            iLog("Unable to parse learnset json ability " + Parsed[i] + " into a valid ability");
        }
    }
    return Abilities;
}

FJsonArrayBP USpecieDataObject::AbilitySetToJsonArray(const TArray<int>& AbilitySet)
{
    const int AbilityCount = AbilitySet.Num();
    TArray<FString> Abilities;
    Abilities.Reserve(AbilityCount);
    for (int i = 0; i < AbilityCount; i++) {
        Abilities.Add(GetAbilityDataManager()->GetAbilityName(AbilitySet[i]).ToString());
    }
    FJsonArrayBP JsonArray;
    JsonArray.SetStringArray("Ability", Abilities);
    return JsonArray;
}

FJsonObjectBP USpecieDataObject::SpecieDataToJson()
{
    FJsonObjectBP Json;
    
    TArray<FName> TypeNames = GetTypeDataManager()->GetTypeNames(TypeBitmask);
    const int TypesCount = TypeNames.Num();
    TArray<FString> TypeStrings;
    TypeStrings.Reserve(TypesCount);
    for (int i = 0; i < TypesCount; i++) {
        TypeStrings.Add(TypeNames[i].ToString());
    }
    FJsonArrayBP TypesArray;
    TypesArray.SetStringArray("Type", TypeStrings);

    Json.SetArrayField(SPECIE_DO_JSON_FIELD_TYPES, TypesArray);

    Json.SetIntegerField(SPECIE_DO_JSON_FIELD_BASE_HEALTH, BaseStats.Health);
    Json.SetIntegerField(SPECIE_DO_JSON_FIELD_BASE_ATTACK, BaseStats.Attack);
    Json.SetIntegerField(SPECIE_DO_JSON_FIELD_BASE_DEFENSE, BaseStats.Defense);
    Json.SetIntegerField(SPECIE_DO_JSON_FIELD_BASE_SPEED, BaseStats.Speed);

    FJsonObjectBP AbilitySets;
    AbilitySets.SetArrayField(SPECIE_DO_JSON_FIELD_LEARNSET_INITIAL, AbilitySetToJsonArray(Learnset.Initial));
    AbilitySets.SetArrayField(SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_10, AbilitySetToJsonArray(Learnset.Level10));
    AbilitySets.SetArrayField(SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_20, AbilitySetToJsonArray(Learnset.Level20));
    AbilitySets.SetArrayField(SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_30, AbilitySetToJsonArray(Learnset.Level30));
    AbilitySets.SetArrayField(SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_40, AbilitySetToJsonArray(Learnset.Level40));
    AbilitySets.SetArrayField(SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_50, AbilitySetToJsonArray(Learnset.Level50));
    AbilitySets.SetArrayField(SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_60, AbilitySetToJsonArray(Learnset.Level60));
    AbilitySets.SetArrayField(SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_70, AbilitySetToJsonArray(Learnset.Level70));
    AbilitySets.SetArrayField(SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_80, AbilitySetToJsonArray(Learnset.Level80));
    AbilitySets.SetArrayField(SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_90, AbilitySetToJsonArray(Learnset.Level90));
    AbilitySets.SetArrayField(SPECIE_DO_JSON_FIELD_LEARNSET_LEVEL_100, AbilitySetToJsonArray(Learnset.Level100));
    Json.SetObjectField(SPECIE_DO_JSON_FIELD_LEARNSET, AbilitySets);

    Json.SetObjectField(SPECIE_DO_JSON_FIELD_SPECIE_SPECIFIC, BP_SpecieDataToJson());

    return Json;
}

FJsonObjectBP USpecieDataObject::BP_SpecieDataToJson_Implementation()
{
    return FJsonObjectBP();
}