// WeaponTypes.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Sound/SoundBase.h" // [필수] 사운드 헤더 추가
#include "GeometryCollection/GeometryCollectionObject.h"
#include "WeaponTypes.generated.h"

// 무기 종류
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
    OneHanded,
    TwoHanded,
    None
};

// 무기 데미지 타입 (날카로움 vs 둔탁함)
UENUM(BlueprintType)
enum class EDamageCategory : uint8
{
    Slash_Pierce, // 도검류 (베기/찌르기)
    Blunt         // 둔기류 (타격)
};

// 무기 스탯 구조체 (데이터 테이블용)
USTRUCT(BlueprintType)
struct FWeaponData : public FTableRowBase
{
    GENERATED_BODY()

    // Initialize pointer to nullptr
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UStaticMesh* WeaponMesh = nullptr;

    // Initialize pointer to nullptr
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UGeometryCollection* FracturedMesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MassKg = 10.f; // 무기 질량 (물리 연산용)

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Durability = 100.f; // 무기 내구도

    // --- 밸런싱 배율 (개별 무기용) ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DamageCoefficient = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ImpulseCoefficient = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AttackSpeedCoefficient = 1.0f;

    // Initialize Enum to a safe default (first value)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EDamageCategory DamageCategory = EDamageCategory::Slash_Pierce; // 데미지 타입

    // Initialize Enum to None
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EWeaponType WeaponType = EWeaponType::None; // 무기 타입
    
    // [신규] 무기 휘두르는 소리 (예: 붕~, 슉!)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
    USoundBase* SwingSound;

    // [신규] 무기로 때렸을 때 나는 소리 (예: 퍽!, 챙강!)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
    USoundBase* HitSound;

};