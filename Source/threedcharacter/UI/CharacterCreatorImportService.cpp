#include "UI/CharacterCreatorImportService.h"

#include "HAL/FileManager.h"
#include "Misc/Paths.h"

namespace
{
    bool MatchesFilter(const FCharacterCreatorAssetCatalogEntry& Entry, const FString& SearchQuery, const FString& CategoryFilter)
    {
        const FString Query = SearchQuery.TrimStartAndEnd().ToLower();
        const FString Category = CategoryFilter.TrimStartAndEnd().ToLower();
        const bool bMatchesSearch = Query.IsEmpty()
            || Entry.AssetName.ToLower().Contains(Query)
            || Entry.RelativePath.ToLower().Contains(Query);
        const bool bMatchesCategory = Category.IsEmpty() || Entry.Category.ToLower() == Category;
        return bMatchesSearch && bMatchesCategory;
    }

    FString MakeKeepBothPath(const FString& DestinationPath)
    {
        const FString Directory = FPaths::GetPath(DestinationPath);
        const FString BaseName = FPaths::GetBaseFilename(DestinationPath);
        const FString Extension = FPaths::GetExtension(DestinationPath, true);
        return FPaths::Combine(Directory, FString::Printf(TEXT("%s_Imported%s"), *BaseName, *Extension));
    }
}

bool FCharacterCreatorImportService::ValidateDirectory(const FString& SourceDirectory, FCharacterCreatorImportProgress& OutProgress)
{
    TArray<FCharacterCreatorAssetCatalogEntry> Entries;
    return ScanDirectory(SourceDirectory, FString(), FString(), Entries, OutProgress);
}

ECharacterCreatorAssetCompatibility FCharacterCreatorImportService::AnalyzeCompatibility(const FString& FilePath, TArray<FText>& OutWarnings)
{
    OutWarnings.Reset();
    const FString Extension = FPaths::GetExtension(FilePath, true);
    const FString LowerPath = FilePath.ToLower();
    if (IFileManager::Get().FileSize(*FilePath) <= 0)
    {
        OutWarnings.Add(FText::FromString(TEXT("The package is empty or unreadable.")));
        return ECharacterCreatorAssetCompatibility::Incompatible;
    }

    if (Extension.Equals(TEXT(".umap"), ESearchCase::IgnoreCase))
    {
        OutWarnings.Add(FText::FromString(TEXT("Map packages may require external level dependencies.")));
        return ECharacterCreatorAssetCompatibility::Warning;
    }

    if (LowerPath.Contains(TEXT("manny")) || LowerPath.Contains(TEXT("mannequin")))
    {
        OutWarnings.Add(FText::FromString(TEXT("Manny source assets require retargeting before Sidekick playback.")));
        return ECharacterCreatorAssetCompatibility::Warning;
    }

    return ECharacterCreatorAssetCompatibility::Compatible;
}

bool FCharacterCreatorImportService::ScanDirectory(const FString& SourceDirectory, const FString& SearchQuery, const FString& CategoryFilter, TArray<FCharacterCreatorAssetCatalogEntry>& OutEntries, FCharacterCreatorImportProgress& OutProgress)
{
    OutEntries.Reset();
    OutProgress = FCharacterCreatorImportProgress();
    OutProgress.SourceDirectory = SourceDirectory;
    OutProgress.State = ECharacterCreatorImportState::Validating;
    OutProgress.Message = FText::FromString(TEXT("Scanning Unreal asset packages..."));

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
        if (Extension.Equals(TEXT(".uasset"), ESearchCase::IgnoreCase) || Extension.Equals(TEXT(".umap"), ESearchCase::IgnoreCase))
        {
            AssetFiles.Add(FilePath);
        }
    }

    OutProgress.TotalFiles = AssetFiles.Num();
    if (AssetFiles.Num() == 0)
    {
        OutProgress.State = ECharacterCreatorImportState::Failed;
        OutProgress.Message = FText::FromString(TEXT("No Unreal asset packages were found in the import directory."));
        return false;
    }

    for (int32 Index = 0; Index < AssetFiles.Num(); ++Index)
    {
        const FString& FilePath = AssetFiles[Index];
        FCharacterCreatorAssetCatalogEntry Entry;
        Entry.SourceFile = FilePath;
        Entry.RelativePath = FilePath;
        FPaths::MakePathRelativeTo(Entry.RelativePath, *SourceDirectory);
        Entry.AssetName = FPaths::GetBaseFilename(FilePath);
        Entry.Extension = FPaths::GetExtension(FilePath, true).ToLower();
        Entry.FileSize = IFileManager::Get().FileSize(*FilePath);
        Entry.Category = Entry.RelativePath.Contains(TEXT("/")) ? Entry.RelativePath.Left(Entry.RelativePath.Find(TEXT("/"))) : TEXT("Root");
        Entry.Compatibility = AnalyzeCompatibility(FilePath, Entry.DependencyWarnings);
        Entry.bConflict = false;

        if (Entry.FileSize <= 0) ++OutProgress.InvalidFiles;
        else ++OutProgress.ValidFiles;
        if (Entry.Compatibility == ECharacterCreatorAssetCompatibility::Compatible) ++OutProgress.CompatibleFiles;
        if (Entry.Compatibility == ECharacterCreatorAssetCompatibility::Warning) ++OutProgress.WarningFiles;
        OutProgress.DependencyWarnings += Entry.DependencyWarnings.Num();

        if (MatchesFilter(Entry, SearchQuery, CategoryFilter))
        {
            OutEntries.Add(Entry);
        }
        OutProgress.Progress = static_cast<float>(Index + 1) / static_cast<float>(OutProgress.TotalFiles);
    }

    OutProgress.State = OutProgress.InvalidFiles > 0 ? ECharacterCreatorImportState::Failed : ECharacterCreatorImportState::Ready;
    OutProgress.Progress = 1.0f;
    OutProgress.Message = FText::FromString(FString::Printf(TEXT("Scanned %d packages • %d compatible • %d warnings"), OutProgress.TotalFiles, OutProgress.CompatibleFiles, OutProgress.WarningFiles));
    return OutProgress.State == ECharacterCreatorImportState::Ready;
}

bool FCharacterCreatorImportService::ImportAssets(const TArray<FCharacterCreatorAssetCatalogEntry>& Entries, const FCharacterCreatorImportOptions& Options, FCharacterCreatorImportProgress& OutProgress)
{
    OutProgress = FCharacterCreatorImportProgress();
    OutProgress.State = ECharacterCreatorImportState::Validating;
    OutProgress.Message = FText::FromString(TEXT("Importing selected asset packages..."));
    OutProgress.TotalFiles = Entries.Num();

    if (Options.DestinationContentDirectory.IsEmpty())
    {
        OutProgress.State = ECharacterCreatorImportState::Failed;
        OutProgress.Message = FText::FromString(TEXT("Choose a destination Content directory before importing."));
        return false;
    }

    for (int32 Index = 0; Index < Entries.Num(); ++Index)
    {
        const FCharacterCreatorAssetCatalogEntry& Entry = Entries[Index];
        if (!Entry.bSelected || Entry.Compatibility == ECharacterCreatorAssetCompatibility::Incompatible)
        {
            ++OutProgress.InvalidFiles;
            continue;
        }

        FString DestinationPath = FPaths::Combine(Options.DestinationContentDirectory, Entry.RelativePath);
        const FString DestinationDirectory = FPaths::GetPath(DestinationPath);
        IFileManager::Get().MakeDirectory(*DestinationDirectory, true);
        if (IFileManager::Get().FileExists(*DestinationPath))
        {
            if (Options.ConflictResolution == ECharacterCreatorImportConflictResolution::Skip && !Options.bOverwriteExisting)
            {
                ++OutProgress.InvalidFiles;
                continue;
            }
            if (Options.ConflictResolution == ECharacterCreatorImportConflictResolution::KeepBoth)
            {
                DestinationPath = MakeKeepBothPath(DestinationPath);
            }
        }

        IFileManager::Get().Copy(*DestinationPath, *Entry.SourceFile, true, true);
        if (IFileManager::Get().FileSize(*DestinationPath) > 0)
        {
            ++OutProgress.ValidFiles;
        }
        else
        {
            ++OutProgress.InvalidFiles;
        }
        OutProgress.Progress = static_cast<float>(Index + 1) / static_cast<float>(FMath::Max(1, OutProgress.TotalFiles));
    }

    OutProgress.State = OutProgress.InvalidFiles == 0 ? ECharacterCreatorImportState::Ready : ECharacterCreatorImportState::Failed;
    OutProgress.Progress = 1.0f;
    OutProgress.Message = FText::FromString(FString::Printf(TEXT("Imported %d of %d selected packages."), OutProgress.ValidFiles, OutProgress.TotalFiles));
    return OutProgress.State == ECharacterCreatorImportState::Ready;
}
