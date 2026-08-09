#include "UI/CharacterCreatorImportService.h"

#include "HAL/FileManager.h"
#include "Misc/Paths.h"

bool FCharacterCreatorImportService::ValidateDirectory(const FString& SourceDirectory, FCharacterCreatorImportProgress& OutProgress)
{
    OutProgress = FCharacterCreatorImportProgress();
    OutProgress.SourceDirectory = SourceDirectory;
    OutProgress.State = ECharacterCreatorImportState::Validating;
    OutProgress.Message = FText::FromString(TEXT("Validating extracted Content assets..."));

    if (SourceDirectory.IsEmpty() || !IFileManager::Get().DirectoryExists(*SourceDirectory))
    {
        OutProgress.State = ECharacterCreatorImportState::Failed;
        OutProgress.Message = FText::FromString(TEXT("Import directory does not exist."));
        return false;
    }

    TArray<FString> Files;
    IFileManager::Get().FindFilesRecursive(Files, *SourceDirectory, TEXT("*.*"), true, false);
    TArray<FString> AssetFiles;
    for (const FString& FilePath : Files)
    {
        const FString Extension = FPaths::GetExtension(FilePath, true);
        if (!Extension.Equals(TEXT(".uasset"), ESearchCase::IgnoreCase) && !Extension.Equals(TEXT(".umap"), ESearchCase::IgnoreCase))
        {
            continue;
        }

        AssetFiles.Add(FilePath);
    }

    OutProgress.TotalFiles = AssetFiles.Num();
    if (OutProgress.TotalFiles == 0)
    {
        OutProgress.State = ECharacterCreatorImportState::Failed;
        OutProgress.Message = FText::FromString(TEXT("No Unreal asset packages were found in the import directory."));
        return false;
    }

    int32 ProcessedFiles = 0;
    for (const FString& FilePath : AssetFiles)
    {
        ++ProcessedFiles;

        if (IFileManager::Get().FileSize(*FilePath) > 0)
        {
            ++OutProgress.ValidFiles;
        }
        else
        {
            ++OutProgress.InvalidFiles;
        }

        OutProgress.Progress = static_cast<float>(ProcessedFiles) / static_cast<float>(OutProgress.TotalFiles);
    }

    if (OutProgress.InvalidFiles > 0)
    {
        OutProgress.State = ECharacterCreatorImportState::Failed;
        OutProgress.Message = FText::FromString(TEXT("One or more Unreal asset packages are empty or corrupted."));
        return false;
    }

    OutProgress.State = ECharacterCreatorImportState::Ready;
    OutProgress.Progress = 1.0f;
    OutProgress.Message = FText::FromString(FString::Printf(TEXT("Validated %d Unreal asset packages."), OutProgress.ValidFiles));
    return true;
}
