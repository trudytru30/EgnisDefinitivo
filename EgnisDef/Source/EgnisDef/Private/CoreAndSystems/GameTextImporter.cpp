#include "CoreAndSystems/GameTextImporter.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

void UGameTextImporter::SyncDataTableFromGoogleSheets(const FString& CSVUrl, UDataTable* TargetDataTable)
{
	if (!TargetDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("[TextImporter]: El DataTable objetivo es NULL."));
		return;
	}

	if (CSVUrl.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[TextImporter]: La URL de Google Sheets está vacía."));
		return;
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    
	Request->OnProcessRequestComplete().BindLambda([TargetDataTable](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
	{
		if (bSuccess && Resp.IsValid() && Resp->GetResponseCode() == 200)
		{
			FString CSVContent = Resp->GetContentAsString();

			TArray<FString> Errors = TargetDataTable->CreateTableFromCSVString(CSVContent);

			if (Errors.Num() == 0)
			{
				TargetDataTable->MarkPackageDirty();
				UE_LOG(LogTemp, Log, TEXT("[TextImporter]: ¡DataTable sincronizado con éxito desde Google Sheets!"));
			}
			else
			{
				for (const FString& Error : Errors)
				{
					UE_LOG(LogTemp, Error, TEXT("[TextImporter]: Error parseando CSV: %s"), *Error);
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[TextImporter]: Error al descargar el CSV de Google Sheets."));
		}
	});

	Request->SetURL(CSVUrl);
	Request->SetVerb("GET");
	Request->ProcessRequest();
}