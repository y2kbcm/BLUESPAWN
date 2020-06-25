#include <Windows.h>

#include <iostream>

#include "util/log/CLISink.h"
#include "user/bluespawn.h"
#include "common/Utils.h"

namespace Log {

	void CLISink::SetConsoleColor(CLISink::MessageColor color){
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hConsole, static_cast<WORD>(color));
	}

	CLISink::CLISink() : hMutex{ CreateMutexW(nullptr, false, L"Local\\CLI-Mutex") } {}
	
	void CLISink::LogMessage(IN CONST LogLevel& level, IN CONST std::wstring& message){
		AcquireMutex mutex{ hMutex };
		if(level.Enabled()){
			SetConsoleColor(CLISink::PrependColors[static_cast<WORD>(level.severity)]);
			std::wcout << CLISink::MessagePrepends[static_cast<WORD>(level.severity)] << " ";
			SetConsoleColor(CLISink::MessageColor::LIGHTGREY);
			std::wcout << message << std::endl;
		}
	}

	bool CLISink::operator==(IN CONST LogSink& sink) const {
		return (bool) dynamic_cast<const CLISink*>(&sink);
	}

	void CLISink::RecordDetection(IN CONST std::reference_wrapper<Detection>& detection, IN RecordType type){
		AcquireMutex mutex{ hMutex };

		if(type == RecordType::PreScan && Bluespawn::EnablePreScanDetections || type == RecordType::PostScan){
			SetConsoleColor(CLISink::PrependColors[4]);
			std::wcout << CLISink::MessagePrepends[4] << (type == RecordType::PreScan ? L"[Pre-Scan] " : L" ");

			EnterCriticalSection(detection.get());
			Detection copy{ detection.get() };
			LeaveCriticalSection(detection.get());

			std::wcout << L"Detection ID: " << copy.dwID << std::endl;

			std::wcout << L"\tDetection Recorded at " << FormatWindowsTime(copy.context.DetectionCreatedTime)
				<< std::endl;
			if(copy.context.note){
				std::wcout << L"\tNote: " << *copy.context.note << std::endl;
			}
			if(copy.context.FirstEvidenceTime){
				std::wcout << L"\tFirst Evidence at " << FormatWindowsTime(*copy.context.FirstEvidenceTime) 
					<< std::endl;
			}

			if(copy.context.hunts.size()){
				std::wcout << L"\tDetected by: ";
				for(auto& hunt : copy.context.hunts){
					std::wcout << hunt << L", ";
				}
				std::wcout << std::endl;
			}
			
			if(copy.DetectionStale){
				std::wcout << L"\tDetection is stale" << std::endl;
			}

			std::wcout << L"\tDetection type: " << (copy.type == DetectionType::FileDetection ? L"File" :
													copy.type == DetectionType::ProcessDetection ? L"Process" :
													copy.type == DetectionType::RegistryDetection ? L"Registry" :
													copy.type == DetectionType::ServiceDetection ? L"Service" :
													std::get<OtherDetectionData>(copy.data).DetectionType) 
				<< std::endl;

			std::wcout << L"\tDetection Certainty: " << static_cast<double>(copy.info.GetCertainty()) << std::endl;
			std::wcout << L"\tDetection Data: " << std::endl;

			auto properties{ copy.Serialize() };
			for(auto& pair : properties){
				std::wcout << L"\t\t" << pair.first << ": " << pair.second << std::endl;
			}
		}
	}

	void CLISink::RecordAssociation(IN CONST std::reference_wrapper<Detection>& first, 
									IN CONST std::reference_wrapper<Detection>& second, IN CONST Association& a){
		AcquireMutex mutex{ hMutex };

		std::cout << "Detections with IDs " << first.get().dwID << " and " << second.get().dwID << " are associated "
			<< " with strength " << static_cast<double>(a) << std::endl;
	}
}