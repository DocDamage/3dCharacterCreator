#include "UI/CharacterCreatorImportService.h"

#include "HAL/FileManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"

namespace
{
    bool IsSafeRelativeAssetPath(const FString& RelativePath)
    {
        if (RelativePath.IsEmpty() || !FPaths::IsRelative(RelativePath)) return false;
        FString Normalized = RelativePath;
        FPaths::NormalizeFilename(Normalized);
        const TArray<FString> Segments = [] (const FString& Path)
        {
            TArray<FString> Values;
            Path.ParseIntoArray(Values, TEXT("/"), true);
            return Values;
        }(Normalized);
        return !Segments.Contains(TEXT("..")) && !Segments.Contains(TEXT("."));
    }

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
        FString Candidate = FPaths::Combine(Directory, FString::Printf(TEXT("%s_Imported%s"), *BaseName, *Extension));
        int32 Suffix = 2;
        while (IFileManager::Get().FileExists(*Candidate))
        {
            Candidate = FPaths::Combine(Directory, FString::Printf(TEXT("%s_Imported_%d%s"), *BaseName, Suffix++, *Extension));
        }
        return Candidate;
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

    const FString ProjectContentDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
    const FString FullSourceDirectory = FPaths::ConvertRelativePathToFull(SourceDirectory);
    const bool bSourceIsMountedContent = FPaths::IsUnderDirectory(FullSourceDirectory, ProjectContentDirectory) || FullSourceDirectory.Equals(ProjectContentDirectory, ESearchCase::IgnoreCase);
    IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    if (bSourceIsMountedContent)
    {
        FString RelativeSource = FullSourceDirectory;
        FPaths::MakePathRelativeTo(RelativeSource, *ProjectContentDirectory);
        RelativeSource.ReplaceInline(TEXT("\\"), TEXT("/"));
        RelativeSource.RemoveFromEnd(TEXT("/"));
        AssetRegistry.ScanPathsSynchronous({ RelativeSource.IsEmpty() ? TEXT("/Game") : TEXT("/Game/") + RelativeSource }, true);
    }

    for (int32 Index = 0; Index < AssetFiles.Num(); ++Index)
    {
        const FString& FilePath = AssetFiles[Index];
        FCharacterCreatorAssetCatalogEntry Entry;
        Entry.SourceFile = FilePath;
        Entry.RelativePath = FilePath;
        FPaths::MakePathRelativeTo(Entry.RelativePath, *SourceDirectory);
        Entry.RelativePath.ReplaceInline(TEXT("\\"), TEXT("/"));
        Entry.AssetName = FPaths::GetBaseFilename(FilePath);
        Entry.Extension = FPaths::GetExtension(FilePath, true).ToLower();
        Entry.FileSize = IFileManager::Get().FileSize(*FilePath);
        Entry.Category = Entry.RelativePath.Contains(TEXT("/")) ? Entry.RelativePath.Left(Entry.RelativePath.Find(TEXT("/"))) : TEXT("Root");
        Entry.Compatibility = AnalyzeCompatibility(FilePath, Entry.DependencyWarnings);
        Entry.bConflict = false;

        if (bSourceIsMountedContent)
        {
            FString RelativeToContent = FPaths::ConvertRelativePathToFull(FilePath);
            FPaths::MakePathRelativeTo(RelativeToContent, *ProjectContentDirectory);
            RelativeToContent.ReplaceInline(TEXT("\\"), TEXT("/"));
            const FString PackageNameString = TEXT("/Game/") + FPaths::ChangeExtension(RelativeToContent, FString());
            const FName PackageName(*PackageNameString);
            TArray<FAssetData> PackageAssets;
            if (AssetRegistry.GetAssetsByPackageName(PackageName, PackageAssets) && PackageAssets.Num() > 0)
            {
                const FAssetData& AssetData = PackageAssets[0];
                Entry.ObjectPath = AssetData.GetSoftObjectPath().ToString();
                Entry.AssetClass = AssetData.AssetClassPath.GetAssetName().ToString();
                Entry.Category = Entry.AssetClass;
            }

            TArray<FName> PackageDependencies;
            AssetRegistry.GetDependencies(PackageName, PackageDependencies, UE::AssetRegistry::EDependencyCategory::Package);
            for (const FName Dependency : PackageDependencies)
            {
                const FString DependencyName = Dependency.ToString();
                Entry.Dependencies.Add(DependencyName);
                if (DependencyName.StartsWith(TEXT("/Game/")) && !FPackageName::DoesPackageExist(DependencyName))
                {
                    Entry.MissingDependencies.Add(DependencyName);
                }
            }
            if (Entry.MissingDependencies.Num() > 0)
            {
                Entry.Compatibility = ECharacterCreatorAssetCompatibility::Incompatible;
                Entry.DependencyWarnings.Add(FText::FromString(FString::Printf(TEXT("%d referenced project packages are missing."), Entry.MissingDependencies.Num())));
            }
        }

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

bool FCharacterCreatorImportService::ScanMountedPath(const FString& PackageRoot, const FString& SearchQuery, const FString& CategoryFilter, TArray<FCharacterCreatorAssetCatalogEntry>& OutEntries, FCharacterCreatorImportProgress& OutProgress)
{
    OutEntries.Reset();
    OutProgress = FCharacterCreatorImportProgress();
    OutProgress.SourceDirectory = PackageRoot;
    OutProgress.State = ECharacterCreatorImportState::Validating;
    OutProgress.Message = FText::FromString(TEXT("Scanning mounted asset registry..."));

    if (!FPackageName::IsValidLongPackageName(PackageRoot, true) || !PackageRoot.StartsWith(TEXT("/Game")))
    {
        OutProgress.State = ECharacterCreatorImportState::Failed;
        OutProgress.Message = FText::FromString(TEXT("The mounted asset path must be a valid /Game package path."));
        return false;
    }

    IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    TArray<FAssetData> Assets;
    AssetRegistry.GetAssetsByPath(FName(*PackageRoot), Assets, true, false);
    Assets.Sort([](const FAssetData& A, const FAssetData& B)
    {
        return A.AssetName.LexicalLess(B.AssetName);
    });

    OutProgress.TotalFiles = Assets.Num();
    for (int32 Index = 0; Index < Assets.Num(); ++Index)
    {
        const FAssetData& AssetData = Assets[Index];
        FCharacterCreatorAssetCatalogEntry Entry;
        Entry.SourceFile = AssetData.PackageName.ToString();
        Entry.RelativePath = Entry.SourceFile;
        Entry.RelativePath.RemoveFromStart(PackageRoot + TEXT("/"));
        Entry.AssetName = AssetData.AssetName.ToString();
        Entry.ObjectPath = AssetData.GetSoftObjectPath().ToString();
        Entry.AssetClass = AssetData.AssetClassPath.GetAssetName().ToString();
        Entry.Category = Entry.AssetClass;
        Entry.Extension = TEXT(".uasset");
        Entry.FileSize = 1; // Mounted IoStore packages do not expose a stable loose-file size.
        Entry.Compatibility = ECharacterCreatorAssetCompatibility::Compatible;

        const FString LowerName = Entry.AssetName.ToLower();
        if (LowerName.Contains(TEXT("manny")) || LowerName.Contains(TEXT("mannequin")))
        {
            Entry.Compatibility = ECharacterCreatorAssetCompatibility::Warning;
            Entry.DependencyWarnings.Add(FText::FromString(TEXT("Manny source assets require retargeting before Sidekick playback.")));
        }

        TArray<FName> PackageDependencies;
        AssetRegistry.GetDependencies(AssetData.PackageName, PackageDependencies, UE::AssetRegistry::EDependencyCategory::Package);
        for (const FName Dependency : PackageDependencies)
        {
            const FString DependencyName = Dependency.ToString();
            Entry.Dependencies.Add(DependencyName);
            if (DependencyName.StartsWith(TEXT("/Game/")) && !FPackageName::DoesPackageExist(DependencyName))
            {
                Entry.MissingDependencies.Add(DependencyName);
            }
        }
        if (Entry.MissingDependencies.Num() > 0)
        {
            Entry.Compatibility = ECharacterCreatorAssetCompatibility::Incompatible;
            Entry.DependencyWarnings.Add(FText::FromString(FString::Printf(TEXT("%d referenced project packages are missing."), Entry.MissingDependencies.Num())));
        }

        ++OutProgress.ValidFiles;
        if (Entry.Compatibility == ECharacterCreatorAssetCompatibility::Compatible) ++OutProgress.CompatibleFiles;
        if (Entry.Compatibility == ECharacterCreatorAssetCompatibility::Warning) ++OutProgress.WarningFiles;
        OutProgress.DependencyWarnings += Entry.DependencyWarnings.Num();
        if (MatchesFilter(Entry, SearchQuery, CategoryFilter))
        {
            OutEntries.Add(MoveTemp(Entry));
        }
        OutProgress.Progress = static_cast<float>(Index + 1) / static_cast<float>(FMath::Max(1, Assets.Num()));
    }

    if (Assets.Num() == 0)
    {
        OutProgress.State = ECharacterCreatorImportState::Failed;
        OutProgress.Progress = 1.0f;
        OutProgress.Message = FText::FromString(FString::Printf(TEXT("No mounted assets were registered under %s."), *PackageRoot));
        return false;
    }

    OutProgress.State = ECharacterCreatorImportState::Ready;
    OutProgress.Progress = 1.0f;
    OutProgress.Message = FText::FromString(FString::Printf(TEXT("Indexed %d mounted assets • %d compatible • %d warnings"), OutProgress.TotalFiles, OutProgress.CompatibleFiles, OutProgress.WarningFiles));
    return true;
}

bool FCharacterCreatorImportService::ImportAssets(const TArray<FCharacterCreatorAssetCatalogEntry>& Entries, const FCharacterCreatorImportOptions& Options, FCharacterCreatorImportProgress& OutProgress)
{
    return ImportAssets(Entries, Options, OutProgress, FProgressCallback(), FShouldCancel());
}

bool FCharacterCreatorImportService::ImportAssets(const TArray<FCharacterCreatorAssetCatalogEntry>& Entries, const FCharacterCreatorImportOptions& Options, FCharacterCreatorImportProgress& OutProgress, const FProgressCallback& ProgressCallback, const FShouldCancel& ShouldCancel)
{
    OutProgress = FCharacterCreatorImportProgress();
    OutProgress.State = ECharacterCreatorImportState::Importing;
    OutProgress.Message = FText::FromString(TEXT("Importing selected asset packages..."));
    for (const FCharacterCreatorAssetCatalogEntry& Entry : Entries)
    {
        if (Entry.bSelected) ++OutProgress.TotalFiles;
    }

    if (Options.DestinationContentDirectory.IsEmpty())
    {
        OutProgress.State = ECharacterCreatorImportState::Failed;
        OutProgress.Message = FText::FromString(TEXT("Choose a destination Content directory before importing."));
        return false;
    }

    if (!IFileManager::Get().MakeDirectory(*Options.DestinationContentDirectory, true) && !IFileManager::Get().DirectoryExists(*Options.DestinationContentDirectory))
    {
        OutProgress.State = ECharacterCreatorImportState::Failed;
        OutProgress.Message = FText::FromString(TEXT("The selected import destination cannot be created."));
        return false;
    }

    if (OutProgress.TotalFiles == 0)
    {
        OutProgress.State = ECharacterCreatorImportState::Failed;
        OutProgress.Message = FText::FromString(TEXT("Select at least one compatible asset before importing."));
        return false;
    }

    for (const FCharacterCreatorAssetCatalogEntry& Entry : Entries)
    {
        if (!Entry.bSelected) continue;
        FCharacterCreatorFileOperationResult& Result = OutProgress.FileResults.AddDefaulted_GetRef();
        Result.SourceFile = Entry.SourceFile;
        OutProgress.CurrentFile = Entry.AssetName;

        if (ShouldCancel && ShouldCancel())
        {
            Result.Outcome = ECharacterCreatorFileOperationOutcome::Cancelled;
            Result.Message = FText::FromString(TEXT("Import cancelled before this file was copied."));
            OutProgress.State = ECharacterCreatorImportState::Cancelled;
            OutProgress.Message = FText::FromString(FString::Printf(TEXT("Import cancelled after %d of %d files."), OutProgress.ProcessedFiles, OutProgress.TotalFiles));
            if (ProgressCallback) ProgressCallback(OutProgress);
            return false;
        }

        if (Entry.Compatibility == ECharacterCreatorAssetCompatibility::Incompatible)
        {
            ++OutProgress.InvalidFiles;
            ++OutProgress.FailedFiles;
            ++OutProgress.ProcessedFiles;
            Result.Outcome = ECharacterCreatorFileOperationOutcome::Failed;
            Result.Message = FText::FromString(TEXT("Skipped because dependency or package validation marked the asset incompatible."));
            OutProgress.Progress = static_cast<float>(OutProgress.ProcessedFiles) / static_cast<float>(OutProgress.TotalFiles);
            if (ProgressCallback) ProgressCallback(OutProgress);
            continue;
        }

        if (!IsSafeRelativeAssetPath(Entry.RelativePath))
        {
            ++OutProgress.InvalidFiles;
            ++OutProgress.FailedFiles;
            ++OutProgress.ProcessedFiles;
            Result.Outcome = ECharacterCreatorFileOperationOutcome::Failed;
            Result.Message = FText::FromString(TEXT("Rejected an unsafe relative package path."));
            OutProgress.Progress = static_cast<float>(OutProgress.ProcessedFiles) / static_cast<float>(OutProgress.TotalFiles);
            if (ProgressCallback) ProgressCallback(OutProgress);
            continue;
        }

        FString DestinationPath = FPaths::Combine(Options.DestinationContentDirectory, Entry.RelativePath);
        const FString FullDestinationRoot = FPaths::ConvertRelativePathToFull(Options.DestinationContentDirectory);
        DestinationPath = FPaths::ConvertRelativePathToFull(DestinationPath);
        if (!FPaths::IsUnderDirectory(DestinationPath, FullDestinationRoot))
        {
            ++OutProgress.InvalidFiles;
            ++OutProgress.FailedFiles;
            ++OutProgress.ProcessedFiles;
            Result.Outcome = ECharacterCreatorFileOperationOutcome::Failed;
            Result.Message = FText::FromString(TEXT("Rejected a destination outside the selected import root."));
            OutProgress.Progress = static_cast<float>(OutProgress.ProcessedFiles) / static_cast<float>(OutProgress.TotalFiles);
            if (ProgressCallback) ProgressCallback(OutProgress);
            continue;
        }
        const FString DestinationDirectory = FPaths::GetPath(DestinationPath);
        IFileManager::Get().MakeDirectory(*DestinationDirectory, true);
        if (IFileManager::Get().FileExists(*DestinationPath))
        {
            if (Options.ConflictResolution == ECharacterCreatorImportConflictResolution::Skip && !Options.bOverwriteExisting)
            {
                ++OutProgress.InvalidFiles;
                ++OutProgress.SkippedFiles;
                ++OutProgress.ProcessedFiles;
                Result.DestinationFile = DestinationPath;
                Result.Outcome = ECharacterCreatorFileOperationOutcome::Skipped;
                Result.Message = FText::FromString(TEXT("Skipped because the destination exists and conflict policy is Skip."));
                OutProgress.Progress = static_cast<float>(OutProgress.ProcessedFiles) / static_cast<float>(OutProgress.TotalFiles);
                if (ProgressCallback) ProgressCallback(OutProgress);
                continue;
            }
            if (Options.ConflictResolution == ECharacterCreatorImportConflictResolution::KeepBoth)
            {
                DestinationPath = MakeKeepBothPath(DestinationPath);
            }
        }

        Result.DestinationFile = DestinationPath;
        const uint32 CopyResult = IFileManager::Get().Copy(*DestinationPath, *Entry.SourceFile, true, true);
        if (CopyResult == COPY_OK && IFileManager::Get().FileSize(*DestinationPath) == Entry.FileSize)
        {
            ++OutProgress.ValidFiles;
            Result.Outcome = ECharacterCreatorFileOperationOutcome::Imported;
            Result.Message = FText::FromString(TEXT("Package copied and size verified."));
        }
        else
        {
            ++OutProgress.InvalidFiles;
            ++OutProgress.FailedFiles;
            Result.Outcome = ECharacterCreatorFileOperationOutcome::Failed;
            Result.Message = FText::FromString(FString::Printf(TEXT("Copy failed with result %u or size verification did not match."), CopyResult));
        }
        ++OutProgress.ProcessedFiles;
        OutProgress.Progress = static_cast<float>(OutProgress.ProcessedFiles) / static_cast<float>(FMath::Max(1, OutProgress.TotalFiles));
        if (ProgressCallback) ProgressCallback(OutProgress);
    }

    OutProgress.State = OutProgress.InvalidFiles == 0 ? ECharacterCreatorImportState::Completed : ECharacterCreatorImportState::CompletedWithErrors;
    OutProgress.Progress = 1.0f;
    OutProgress.Message = FText::FromString(FString::Printf(TEXT("Imported %d of %d selected packages • %d skipped • %d failed."), OutProgress.ValidFiles, OutProgress.TotalFiles, OutProgress.SkippedFiles, OutProgress.FailedFiles));
    if (ProgressCallback) ProgressCallback(OutProgress);
    return OutProgress.State == ECharacterCreatorImportState::Completed;
}
