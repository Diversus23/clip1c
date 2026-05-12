#include "ClipboardManager.h"

#ifdef __APPLE__

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

#include <vector>
#include <string>

BaseHelper::ClipboardManager::ClipboardManager()
{
}

BaseHelper::ClipboardManager::~ClipboardManager()
{
}

std::string BaseHelper::ClipboardManager::GetFormat()
{
	JSON json;
	@autoreleasepool {
		NSPasteboard* pb = [NSPasteboard generalPasteboard];
		NSArray<NSPasteboardType>* types = [pb types];
		NSUInteger index = 0;
		for (NSPasteboardType t in types) {
			JSON j;
			j["key"] = (int)(++index);
			j["name"] = std::string([t UTF8String]);
			json.push_back(j);
		}
	}
	return json.dump();
}

std::wstring BaseHelper::ClipboardManager::GetText()
{
	std::string text;
	@autoreleasepool {
		NSPasteboard* pb = [NSPasteboard generalPasteboard];
		NSString* s = [pb stringForType:NSPasteboardTypeString];
		if (s) text = [s UTF8String];
	}
	return MB2WC(text);
}

bool BaseHelper::ClipboardManager::SetText(const std::wstring& text, bool bEmpty)
{
	@autoreleasepool {
		NSPasteboard* pb = [NSPasteboard generalPasteboard];
		if (bEmpty) [pb clearContents];
		NSString* s = [NSString stringWithUTF8String:WC2MB(text).c_str()];
		if (!s) return false;
		return [pb setString:s forType:NSPasteboardTypeString] == YES;
	}
}

std::wstring BaseHelper::ClipboardManager::GetFiles()
{
	JSON json;
	@autoreleasepool {
		NSPasteboard* pb = [NSPasteboard generalPasteboard];
		NSArray* classes = @[[NSURL class]];
		NSDictionary* options = @{ NSPasteboardURLReadingFileURLsOnlyKey: @YES };
		NSArray<NSURL*>* urls = [pb readObjectsForClasses:classes options:options];
		for (NSURL* u in urls) {
			if (u.isFileURL) {
				json.push_back(std::string([[u path] UTF8String]));
			}
		}
	}
	std::string s = json.dump();
	return MB2WC(s);
}

bool BaseHelper::ClipboardManager::SetFiles(const std::string& text, bool bEmpty)
{
	@autoreleasepool {
		nlohmann::json arr;
		try { arr = nlohmann::json::parse(text); }
		catch (...) { return false; }
		if (!arr.is_array()) return false;

		NSMutableArray<NSURL*>* urls = [NSMutableArray array];
		for (auto& item : arr) {
			if (!item.is_string()) continue;
			NSString* path = [NSString stringWithUTF8String:item.get<std::string>().c_str()];
			if (!path) continue;
			NSURL* url = [NSURL fileURLWithPath:path];
			if (url) [urls addObject:url];
		}
		NSPasteboard* pb = [NSPasteboard generalPasteboard];
		if (bEmpty) [pb clearContents];
		return [pb writeObjects:urls] == YES;
	}
}

bool BaseHelper::ClipboardManager::GetImage(VH& data)
{
	@autoreleasepool {
		NSPasteboard* pb = [NSPasteboard generalPasteboard];
		NSData* png = [pb dataForType:NSPasteboardTypePNG];
		if (!png) {
			NSData* tiff = [pb dataForType:NSPasteboardTypeTIFF];
			if (!tiff) return true;
			NSBitmapImageRep* rep = [NSBitmapImageRep imageRepWithData:tiff];
			if (!rep) return true;
			png = [rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
			if (!png) return true;
		}
		NSUInteger len = [png length];
		if (len == 0) return true;
		data.AllocMemory((unsigned long)len);
		memcpy(data.data(), [png bytes], len);
		return true;
	}
}

bool BaseHelper::ClipboardManager::SetImage(VH& data, bool bEmpty)
{
	@autoreleasepool {
		NSData* png = [NSData dataWithBytes:data.data() length:data.size()];
		if (!png) return false;
		NSPasteboard* pb = [NSPasteboard generalPasteboard];
		if (bEmpty) [pb clearContents];
		// Записываем оба формата: PNG (как есть) и TIFF (для совместимости со стандартными приложениями macOS).
		BOOL ok = [pb setData:png forType:NSPasteboardTypePNG];
		NSBitmapImageRep* rep = [NSBitmapImageRep imageRepWithData:png];
		if (rep) {
			NSData* tiff = [rep TIFFRepresentation];
			if (tiff) [pb setData:tiff forType:NSPasteboardTypeTIFF];
		}
		return ok == YES;
	}
}

bool BaseHelper::ClipboardManager::Empty()
{
	@autoreleasepool {
		[[NSPasteboard generalPasteboard] clearContents];
	}
	return true;
}

#endif //__APPLE__
