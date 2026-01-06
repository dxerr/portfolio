# MetaHumanMaterialLODMaster 설계 문서

## 1. Skeletal Mesh를 위한 C++ 로직

LOD별 머티리얼 할당을 관리하기 위해 `FSkeletalMeshLODInfo::LODMaterialMap`을 수정합니다. 이를 통해 특정 LOD의 특정 섹션이 전역 `USkeletalMesh::Materials` 목록에 있는 기본 머티리얼 인덱스 대신 다른 머티리얼 인덱스를 사용하도록 리다이렉트할 수 있습니다.

### 코드 스니펫: `SkeletalMeshLODManager.cpp`

```cpp
#include "Engine/SkeletalMesh.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "Engine/SkinnedAssetCommon.h"

/**
 * 특정 LOD 내의 섹션에 특정 머티리얼 인터페이스를 적용합니다.
 * 
 * @param SkeletalMesh      대상 에셋.
 * @param LODIndex          LOD 레벨 (0-7).
 * @param SectionIndex      LOD 내 섹션 인덱스 (단일 메쉬의 경우 보통 0이지만, 여러 개일 수 있음).
 * @param NewMaterial       할당할 머티리얼 인터페이스.
 */
void UpgradeSkeletalMeshLODMaterial(USkeletalMesh* SkeletalMesh, int32 LODIndex, int32 SectionIndex, UMaterialInterface* NewMaterial)
{
    if (!SkeletalMesh || !NewMaterial) return;

    // 유효한 LOD 정보가 있는지 확인
    if (!SkeletalMesh->IsValidLODIndex(LODIndex)) return;
    
    FSkeletalMeshLODInfo* LODInfo = SkeletalMesh->GetLODInfo(LODIndex);
    if (!LODInfo) return;

    // 1. 전역 머티리얼 목록에서 해당 머티리얼을 찾거나 추가
    int32 MaterialIndex = INDEX_NONE;
    const TArray<FSkeletalMaterial>& Materials = SkeletalMesh->GetMaterials();
    
    // 이미 존재하는지 확인
    for (int32 i = 0; i < Materials.Num(); ++i)
    {
        if (Materials[i].MaterialInterface == NewMaterial)
        {
            MaterialIndex = i;
            break;
        }
    }

    // 없다면 추가
    if (MaterialIndex == INDEX_NONE)
    {
        SkeletalMesh->GetMaterials().Add(FSkeletalMaterial(NewMaterial));
        MaterialIndex = SkeletalMesh->GetMaterials().Num() - 1;
    }

    // 2. LODMaterialMap 업데이트
    // LODMaterialMap은 섹션 인덱스 -> 전역 머티리얼 인덱스로 매핑합니다.
    // 키(배열 크기)가 섹션 인덱스를 포함하도록 확인
    if (LODInfo->LODMaterialMap.Num() <= SectionIndex)
    {
        LODInfo->LODMaterialMap.SetNum(SectionIndex + 1);
        // 리매핑(INDEX_NONE)이나 기본 동작을 피하기 위해 새 항목 초기화
        for (int32 i = 0; i < LODInfo->LODMaterialMap.Num(); ++i)
        {
             if (i != SectionIndex && LODInfo->LODMaterialMap[i] == 0) // 기본 초기화 체크
             {
                 LODInfo->LODMaterialMap[i] = INDEX_NONE; 
             }
        }
    }

    LODInfo->LODMaterialMap[SectionIndex] = MaterialIndex;

    // 3. 변경 사항 적용
    SkeletalMesh->PostEditChange();
    SkeletalMesh->MarkPackageDirty();
    
    // 렌더 리소스 강제 재초기화 (에디터 시각적 업데이트에 중요)
    SkeletalMesh->InitResources();
}
```

## 2. Groom (Hair)을 위한 C++ 로직

Groom 에셋은 "Strands"(주로 LOD 0/1)와 "Cards" 및 "Meshes"(LOD 2+)를 분리합니다.
Cards에 대한 머티리얼을 오버라이드하려면, 특정 LOD의 Card 데이터에 `MaterialSlotName`을 할당하고, 해당 슬롯 이름이 Groom의 머티리얼 목록에 존재하는지 확인해야 합니다.

### 코드 스니펫: `GroomLODManager.cpp`

```cpp
#include "GroomAsset.h"
#include "GroomAssetCards.h"

/**
 * 특정 LOD의 Cards 지오메트리에 머티리얼을 할당합니다.
 * 
 * @param GroomAsset    대상 Groom 에셋.
 * @param GroupIndex    헤어 그룹 인덱스 (단일 헤어의 경우 보통 0).
 * @param LODIndex      Cards가 사용되는 LOD 레벨.
 * @param CardMaterial  Cards에 적용할 머티리얼.
 */
void SetGroomCardMaterial(UGroomAsset* GroomAsset, int32 GroupIndex, int32 LODIndex, UMaterialInterface* CardMaterial)
{
    if (!GroomAsset || !CardMaterial) return;

    // 1. 이 오버라이드를 위한 고유 슬롯 이름 정의
    // 명명 규칙: Cards_GroupX_LODY
    FString SlotNameStr = FString::Printf(TEXT("Cards_Group%d_LOD%d"), GroupIndex, LODIndex);
    FName SlotName(*SlotNameStr);

    // 2. HairGroupsMaterials에 머티리얼 슬롯 추가 또는 업데이트
    bool bFoundSlot = false;
    TArray<FHairGroupsMaterial>& Materials = GroomAsset->GetHairGroupsMaterials();
    
    for (FHairGroupsMaterial& Entry : Materials)
    {
        if (Entry.SlotName == SlotName)
        {
            Entry.Material = CardMaterial;
            bFoundSlot = true;
            break;
        }
    }

    if (!bFoundSlot)
    {
        FHairGroupsMaterial NewEntry;
        NewEntry.SlotName = SlotName;
        NewEntry.Material = CardMaterial;
        Materials.Add(NewEntry);
    }

    // 3. 해당 슬롯 이름을 이 LOD의 Cards 지오메트리 구성에 할당
    TArray<FHairGroupsCardsSourceDescription>& CardsSource = GroomAsset->GetHairGroupsCards();
    bool bUpdated = false;

    for (FHairGroupsCardsSourceDescription& CardDesc : CardsSource)
    {
        // 이 Card 설명이 우리 그룹과 LOD에 적용되는지 확인
        // 참고: 로직은 Groom이 Cards를 어떻게 설정했는지에 따라 다름. 
        // 종종 SourceDescription의 'LODIndex'가 -1(모두 유효)이거나 특정 값일 수 있음.
        if (CardDesc.GroupIndex == GroupIndex && (CardDesc.LODIndex == LODIndex || CardDesc.LODIndex == -1))
        {
            CardDesc.MaterialSlotName = SlotName;
            bUpdated = true;
            // -1(공유)이었다면, 이 LOD를 위해 이 항목을 구체적으로 복제하고 싶을 수도 있지만
            // 여기서는 단순화를 위해 1:1 매핑이나 사용자가 전체 오버라이드를 의도했다고 가정함.
        }
    }

    if (bUpdated)
    {
        GroomAsset->Modify();
        GroomAsset->UpdateResource(); // 렌더 데이터 재빌드 트리거
        GroomAsset->MarkPackageDirty();
    }
}
```

## 3. UI 컨셉

Data Grid가 포함된 **Editor Utility Widget**을 사용할 것입니다.

### 레이아웃 설명
*   **상단 바**:
    *   `SinglePropertyView`: `USkeletalMesh` 또는 `UGroomAsset` 선택.
    *   `Button`: "새로고침" (Refresh).
*   **메인 콘텐츠 (Skeletal Mesh 모드)**:
    *   **데이터 테이블 / 그리드**:
        *   **헤더 행**: 섹션 0 | 섹션 1 | 섹션 2 ...
        *   **행**:
            *   LOD 0
            *   LOD 1
            *   ...
            *   LOD 7
        *   **셀 콘텐츠**: `ContentWidget` -> `AssetPicker` (MaterialInterface로 필터링).
            *   현재 머티리얼 표시.
            *   OnChanged -> `UpgradeSkeletalMeshLODMaterial` 호출.
*   **메인 콘텐츠 (Groom 모드)**:
    *   **데이터 테이블**:
        *   **행**: LOD 레벨.
        *   **열**: "지오메트리 타입" (Strands vs Cards), "머티리얼 오버라이드".
        *   `지오메트리 타입`: 읽기 전용 텍스트 (예: "Cards").
        *   `머티리얼 오버라이드`: `AssetPicker` -> `SetGroomCardMaterial` 호출.

### UMG 계층 구조 개념 (트리 뷰)
```text
[Border] 배경
  [VerticalBox]
    [HorizontalBox] 헤더
       [DetailsView] 에셋 선택기 (AssetSelector)
       [Button] 새로고침 (Refresh)
    
    [ScrollBox] 메인 영역 (MainArea)
       [UniformGridPanel] 머티리얼 그리드 (MaterialGrid)
          // C++ 또는 블루프린트에서 동적으로 생성
          // 행 0: 헤더 (섹션 인덱스)
          // 행 1..8: LOD 0..7 컨트롤
```

## 4. 설치 및 컴파일 방법 (Installation)

이 플러그인은 C++ 소스 코드로 제공되므로, 프로젝트를 **재빌드(Rebuild)**해야 사용할 수 있습니다.

1.  **플러그인 위치 확인**:
    *   `[프로젝트폴더]/Plugins/MetaHumanMaterialLODMaster` 폴더에 소스 코드가 생성되었습니다.
2.  **프로젝트 파일 생성**:
    *   `.uproject` 파일을 우클릭하고 **Generate Visual Studio project files**를 실행합니다.
3.  **컴파일**:
    *   Visual Studio 솔루션(`.sln`)을 열고 프로젝트를 빌드합니다.
    *   또는 언리얼 에디터를 실행할 때 "Missing Modules" 팝업이 뜨면 **Yes**를 눌러 빌드합니다.
4.  **활성화 확인**:
    *   에디터 실행 후 `Edit > Plugins` 메뉴에서 **MetaHuman Material LOD Master**가 활성화(Enabled)되어 있는지 확인합니다.

## 5. 사용 방법 (How to Use)

플러그인이 로드되면 레벨 에디터 상단 툴바에 아이콘이 추가되거나, `Window > MetaHuman Material LOD Master` 메뉴를 통해 창을 열 수 있습니다.

1.  **Skeletal Mesh 작업**:
    *   플러그인 창의 상단 "Asset Picker"에서 **MetaHuman Face Mesh** (Skeletal Mesh)를 선택합니다.
    *   LOD 리스트가 표시되면, 특정 LOD 행(예: LOD 3)의 드롭다운에서 원하는 **Material Interface**를 선택합니다.
    *   선택 즉시 해당 LOD 세머티리얼이 변경되고 에셋이 수정(Dirty)됩니다.
    *   에셋을 저장합니다.

2.  **Groom (헤어) 작업**:
    *   "Asset Picker"에서 **Groom Asset**을 선택합니다.
    *   LOD 리스트(Cards/Meshes 단계)에 대해 **Material Interface**를 선택합니다.
    *   자동으로 플러그인이 `Cards_Group0_LODx`와 같은 슬롯 이름을 생성하고 매핑을 처리합니다.
    *   에셋을 저장합니다.
