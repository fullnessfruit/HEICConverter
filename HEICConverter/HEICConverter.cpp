// HEICConverter.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "pch.h"
#include <iostream>
#include <filesystem>
#include <wincodec.h>

int main()
{
	using std::experimental::filesystem::v1::directory_iterator;
	using std::experimental::filesystem::v1::path;

	const enum class Format
	{
		INVALID,
		BMP,
		PNG,
		JPG,
	} format = Format::JPG;

	WICPixelFormatGUID pixelFormatGUID;
	int channelCount;
	std::string extension;

	// https://docs.microsoft.com/en-us/windows/win32/wic/-wic-codec-native-pixel-formats
	// pixelFormatGUID 를 저장할 포맷에 따라 다르게 설정해서, 원본 파일에서 비트맵 데이터를 읽어올 때, 저장할 파일 포맷이 지원하는 픽셀 포맷으로 읽어온다.
	switch (format)
	{
	case Format::BMP:
		pixelFormatGUID = GUID_WICPixelFormat32bppBGRA;
		channelCount = 4;
		extension = ".bmp";
		break;

	case Format::PNG:
		pixelFormatGUID = GUID_WICPixelFormat32bppBGRA;
		channelCount = 4;
		extension = ".png";
		break;

	case Format::JPG:
		pixelFormatGUID = GUID_WICPixelFormat24bppBGR;
		channelCount = 3;	// 알파 채널은 없고 BGR만 있으므로 3
		extension = ".jpg";
		break;

	default:
		return 1;
	}

	HRESULT result = E_FAIL;

	result = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (result != S_OK)
	{
		// TODO: Error log.
		return 2;
	}

	// Visual Studio 2017에는 std::filesystem 이 도입되지 않은 것 같다.
	for (auto &i : directory_iterator("."))
	{
		// 대문자이므로 대문자인 경우만 처리된다.
		if (i.path().extension().compare(".HEIC") != 0)
		{
			continue;
		}

		IWICImagingFactory *imagingFactory;

		result = ::CoCreateInstance(CLSID_WICImagingFactory1, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&imagingFactory));
		if (result != S_OK)
		{
			// TODO: Error log.
			return 3;
		}

		IWICBitmapDecoder *bitmapDecoder;

		result = imagingFactory->CreateDecoderFromFilename(i.path().wstring().c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &bitmapDecoder);
		if (result != S_OK)
		{
			// TODO: Error log.
			return 4;
		}

		IWICBitmapFrameDecode *bitmapFrameDecode;
		IWICFormatConverter *formatConverter;

		if (bitmapDecoder == nullptr)
		{
			// TODO: Error log.
			return 5;
		}

		result = bitmapDecoder->GetFrame(0, &bitmapFrameDecode);
		if (result != S_OK)
		{
			// TODO: Error log.
			return 6;
		}

		result = imagingFactory->CreateFormatConverter(&formatConverter);
		if (result != S_OK)
		{
			// TODO: Error log.
			return 7;
		}

		result = formatConverter->Initialize(bitmapFrameDecode, pixelFormatGUID, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);
		if (result != S_OK)
		{
			// TODO: Error log.
			return 8;
		}

		UINT pcActualCount;
		UINT tActualCount;

		result = bitmapFrameDecode->GetColorContexts(0, nullptr, &pcActualCount);
		if (result != S_OK)
		{
			// TODO: Error log.
			return 9;
		}

		IWICColorContext **colorContext = new IWICColorContext *[pcActualCount];

		for (auto i = 0u; i < pcActualCount; i++)
		{
			result = imagingFactory->CreateColorContext(&colorContext[i]);
			if (result != S_OK)
			{
				// TODO: Error log.
				return 10;
			}
		}

		result = bitmapFrameDecode->GetColorContexts(pcActualCount, colorContext, &tActualCount);
		if (result != S_OK)
		{
			// TODO: Error log.
			return 11;
		}

		// 변환되는 파일로 metadata 정보도 옮기고 싶은 경우 아래 코드를 적절히 수정해서 원본 파일에서 필요한 metadata 정보를 읽는다. metadata 구조는 파일 포맷마다 다르므로 metadata 정보를 옮기고 싶다면 해당 파일 포맷에 대한 문서를 참조해서 데이터를 읽어와서 저장할 파일에 적절히 입력해야 할 것 같다. 만약 서로 다른 파일 포맷이지만 같은 EXIF를 사용하고 있다거나 하는 등의 서로 공통점이 있다면 옮기기가 좀 더 수월해질지도 모른다.
		// https://docs.microsoft.com/en-us/windows/win32/wic/-wic-native-image-format-metadata-queries
		//IWICMetadataQueryReader *metadataQueryReader;

		//result = bitmapFrameDecode->GetMetadataQueryReader(&metadataQueryReader);
		//if (result != S_OK)
		//{
		//	// TODO: Error log.
		//	return 12;
		//}

		//if (!metadataQueryReader)
		//{
		//	// TODO: Error log.
		//	return 13;
		//}

		//PROPVARIANT value;

		//// 값이 없는 경우 result 에 WINCODEC_ERR_PROPERTYNOTFOUND 가 대입되는데 이 경우에 대한 처리를 해야함.
		//PropVariantInit(&value);
		//result = metadataQueryReader->GetMetadataByName(L"/ifd/exif/{ushort=40961}", &value);	// EXIF의 칼라 프로파일 정보의 상대 경로는 {ushort = 40961} 이다. 절대 경로는 HEIF 문서를 못찾아서, 적당히 입력해보다 우연히 찾아냈다.
		//if (result != S_OK)
		//{
		//	// TODO: Error log.
		//	return 14;
		//}

		//metadataQueryReader->Release();
		//metadataQueryReader = nullptr;

		unsigned int width;
		unsigned int height;
		unsigned char *data;

		result = formatConverter->GetSize(&width, &height);
		if (result != S_OK)
		{
			// TODO: Error log.
			return 15;
		}

		data = new unsigned char[width * height * channelCount];
		if (!data)
		{
			// TODO: Error log.
			return 16;
		}

		result = formatConverter->CopyPixels(nullptr, width * channelCount, width * height * channelCount, data);
		if (result != S_OK)
		{
			// TODO: Error log.
			return 17;
		}

		// https://docs.microsoft.com/en-us/windows/win32/wic/-wic-creating-encoder
		IWICStream *stream = nullptr;

		result = imagingFactory->CreateStream(&stream);
		if (result != S_OK)
		{
			// TODO: Error log.
			return 18;
		}

		if (!stream)
		{
			// TODO: Error log.
			return 19;
		}

		path tPath = i.path();

		result = stream->InitializeFromFilename(tPath.replace_extension(extension).wstring().c_str(), GENERIC_WRITE);
		if (result != S_OK)
		{
			// TODO: Error log.
			return 20;
		}

		IWICBitmapEncoder *bitmapEncoder = nullptr;

		switch (format)
		{
		case Format::BMP:
			result = imagingFactory->CreateEncoder(GUID_ContainerFormatBmp, nullptr, &bitmapEncoder);
			break;

		case Format::PNG:
			result = imagingFactory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &bitmapEncoder);
			break;

		case Format::JPG:
			result = imagingFactory->CreateEncoder(GUID_ContainerFormatJpeg, nullptr, &bitmapEncoder);
			break;

		default:
			return 21;
		}
		if (result != S_OK)
		{
			// TODO: Error log.
			return 22;
		}

		if (!bitmapEncoder)
		{
			// TODO: Error log.
			return 23;
		}

		result = bitmapEncoder->Initialize(stream, WICBitmapEncoderNoCache);
		if (result != S_OK)
		{
			// TODO: Error log.
			return 24;
		}

		IWICBitmapFrameEncode *bitmapFrameEncode = nullptr;
		IPropertyBag2 *propertyBag2 = nullptr;

		result = bitmapEncoder->CreateNewFrame(&bitmapFrameEncode, &propertyBag2);
		if (result != S_OK)
		{
			// TODO: Error log.
			return 25;
		}

		if (!bitmapFrameEncode)
		{
			// TODO: Error log.
			return 26;
		}

		if (!propertyBag2)
		{
			// TODO: Error log.
			return 27;
		}

		// 왜 그런지는 모르겠지만 무조건 하나 이상의 값을 설정해줘야 정상적으로 동작하는 것 같다.
		switch (format)
		{
		case Format::BMP:
		{
			// https://docs.microsoft.com/en-us/windows/win32/wic/bmp-format-overview
			PROPBAG2 option = { 0 };
			VARIANT varValue;	// VARIANT 를 F12로 헤더로 가보면 vt값에 해당하는 타입을 알 수 있으므로 그에 맞는 union 변수를 쓰면 된다.

			// 왜 안되는지 모르겠어서 const_cast 를 사용함.
			option.pstrName = const_cast<LPOLESTR>(L"EnableV5Header32bppBGRA");
			VariantInit(&varValue);
			varValue.vt = VT_BOOL;
			varValue.boolVal = VARIANT_FALSE;
			result = propertyBag2->Write(1, &option, &varValue);

			result = bitmapFrameEncode->Initialize(propertyBag2);
			if (result != S_OK)
			{
				// TODO: Error log.
				return 28;
			}
		}
		break;

		case Format::PNG:
		{
			// https://docs.microsoft.com/en-us/windows/win32/wic/png-format-overview
			PROPBAG2 option = { 0 };
			VARIANT varValue;	// VARIANT 를 F12로 헤더로 가보면 vt값에 해당하는 타입을 알 수 있으므로 그에 맞는 union 변수를 쓰면 된다.

			// 왜 안되는지 모르겠어서 const_cast 를 사용함.
			option.pstrName = const_cast<LPOLESTR>(L"InterlaceOption");
			VariantInit(&varValue);
			varValue.vt = VT_BOOL;
			varValue.boolVal = FALSE;
			result = propertyBag2->Write(1, &option, &varValue);

			result = bitmapFrameEncode->Initialize(propertyBag2);
			if (result != S_OK)
			{
				// TODO: Error log.
				return 29;
			}

			// 칼라 프로파일 설정을 하지 않으면 원본 파일에서 보던 색감과 다르게 보일 것이다.
			result = bitmapFrameEncode->SetColorContexts(pcActualCount, colorContext);
			if (result != S_OK)
			{
				// TODO: Error log.
				return 30;
			}
		}
		break;

		case Format::JPG:
		{
			// https://docs.microsoft.com/en-us/windows/win32/wic/jpeg-format-overview
			PROPBAG2 option = { 0 };
			VARIANT varValue;	// VARIANT 를 F12로 헤더로 가보면 vt값에 해당하는 타입을 알 수 있으므로 그에 맞는 union 변수를 쓰면 된다.

			// 왜 안되는지 모르겠어서 const_cast 를 사용함.
			option.pstrName = const_cast<LPOLESTR>(L"ImageQuality");
			VariantInit(&varValue);
			varValue.vt = VT_R4;
			varValue.fltVal = 1.0;	// 이 값이 이미지의 품질인 듯 하다. 1.0이 제일 좋은 화질인 듯 하다.
			result = propertyBag2->Write(1, &option, &varValue);

			result = bitmapFrameEncode->Initialize(propertyBag2);
			if (result != S_OK)
			{
				// TODO: Error log.
				return 31;
			}

			// 칼라 프로파일 설정을 하지 않으면 원본 파일에서 보던 색감과 다르게 보일 것이다.
			result = bitmapFrameEncode->SetColorContexts(pcActualCount, colorContext);
			if (result != S_OK)
			{
				// TODO: Error log.
				return 32;
			}

			// metadata 에도 칼라 프로파일 정보를 입력하고 싶다면 아래 코드가 동작하도록 수정해야 한다. 이상하게 IWICBitmapFrameEncode::SetColorContexts()로 칼라 프로파일 설정을 하면 metadata의 칼라 프로파일 정보가 사라진다.
			//IWICMetadataQueryWriter *metadataQueryWriter;

			//result = bitmapFrameEncode->GetMetadataQueryWriter(&metadataQueryWriter);
			//if (result != S_OK)
			//{
			//	// TODO: Error log.
			//	return 33;
			//}

			//if (!metadataQueryWriter)
			//{
			//	// TODO: Error log.
			//	return 34;
			//}

			//result = metadataQueryWriter->SetMetadataByName(L"/app1/ifd/exif/{ushort=40961}", &value);
			//if (result != S_OK)
			//{
			//	// TODO: Error log.
			//	return 35;
			//}

			//PropVariantClear(&value);
			//metadataQueryWriter->Release();
			//metadataQueryWriter = nullptr;
		}
		break;

		default:
			return 36;
		}

		result = bitmapFrameEncode->SetSize(width, height);
		if (result != S_OK)
		{
			// TODO: Error log.
			return 37;
		}

		result = bitmapFrameEncode->SetPixelFormat(&pixelFormatGUID);
		if (result != S_OK)
		{
			// TODO: Error log.
			return 38;
		}

		UINT cbStride = width * channelCount;
		UINT cbBufferSize = height * cbStride;

		result = bitmapFrameEncode->WritePixels(height, cbStride, cbBufferSize, data);
		if (result != S_OK)
		{
			// TODO: Error log.
			delete[] data;
			return 39;
		}

		delete[] data;

		result = bitmapFrameEncode->Commit();
		if (result != S_OK)
		{
			// TODO: Error log.
			return 40;
		}

		result = bitmapEncoder->Commit();
		if (result != S_OK)
		{
			// TODO: Error log.
			return 41;
		}

		imagingFactory->Release();
		imagingFactory = nullptr;
		bitmapDecoder->Release();
		bitmapDecoder = nullptr;
		bitmapFrameDecode->Release();
		bitmapFrameDecode = nullptr;
		formatConverter->Release();
		formatConverter = nullptr;
		for (auto i = 0u; i < pcActualCount; i++)
		{
			colorContext[i]->Release();
			colorContext[i] = nullptr;
		}
		delete[] colorContext;
		colorContext = nullptr;
		stream->Release();
		stream = nullptr;
		bitmapEncoder->Release();
		bitmapEncoder = nullptr;
		bitmapFrameEncode->Release();
		bitmapFrameEncode = nullptr;
		propertyBag2->Release();
		propertyBag2 = nullptr;
	}

	return 0;
}

// 프로그램 실행: <Ctrl+F5> 또는 [디버그] > [디버깅하지 않고 시작] 메뉴
// 프로그램 디버그: <F5> 키 또는 [디버그] > [디버깅 시작] 메뉴

// 시작을 위한 팁: 
//   1. [솔루션 탐색기] 창을 사용하여 파일을 추가/관리합니다.
//   2. [팀 탐색기] 창을 사용하여 소스 제어에 연결합니다.
//   3. [출력] 창을 사용하여 빌드 출력 및 기타 메시지를 확인합니다.
//   4. [오류 목록] 창을 사용하여 오류를 봅니다.
//   5. [프로젝트] > [새 항목 추가]로 이동하여 새 코드 파일을 만들거나, [프로젝트] > [기존 항목 추가]로 이동하여 기존 코드 파일을 프로젝트에 추가합니다.
//   6. 나중에 이 프로젝트를 다시 열려면 [파일] > [열기] > [프로젝트]로 이동하고 .sln 파일을 선택합니다.
