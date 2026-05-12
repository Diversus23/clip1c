#ifdef __APPLE__

// Apple-фреймворки импортируем ПЕРВЫМИ — они определяют BOOL как signed char
// и выставляют OBJC_BOOL_DEFINED, что блокирует определение BOOL=int в com.h
// (1С SDK). Иначе получаем typedef redefinition.
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#include <dispatch/dispatch.h>

#include "ClipboardManager.h"

#include <vector>
#include <string>

// AppKit (NSBitmapImageRep, NSImage) thread-unsafe вне main thread. 1С зовёт компоненту
// из своего фонового потока, поэтому каждый вход в AppKit оборачиваем в синхронный диспатч
// на main queue. Если уже на main — вызываем напрямую, чтобы не словить deadlock.
static void runOnMainSync(dispatch_block_t block)
{
	if ([NSThread isMainThread]) {
		block();
	} else {
		dispatch_sync(dispatch_get_main_queue(), block);
	}
}

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
		if (!pb) return MB2WC(json.dump());
		NSArray* classes = @[[NSURL class]];
		NSDictionary* options = @{ NSPasteboardURLReadingFileURLsOnlyKey: @YES };
		NSArray<NSURL*>* urls = [pb readObjectsForClasses:classes options:options];
		for (NSURL* u in urls) {
			if (!u.isFileURL) continue;
			NSString* path = [u path];
			if (!path) continue;
			const char* utf8 = [path UTF8String];
			if (!utf8) continue;
			json.push_back(std::string(utf8));
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
	__block bool result = true;
	// AppKit (NSBitmapImageRep, TIFF→PNG конвертация) требует main thread.
	runOnMainSync(^{
		@autoreleasepool {
			NSPasteboard* pb = [NSPasteboard generalPasteboard];
			if (!pb) { result = false; return; }
			NSData* png = [pb dataForType:NSPasteboardTypePNG];
			if (!png) {
				NSData* tiff = [pb dataForType:NSPasteboardTypeTIFF];
				if (!tiff) return; // буфер пуст — это не ошибка, data остаётся пустым.
				NSBitmapImageRep* rep = [NSBitmapImageRep imageRepWithData:tiff];
				if (!rep) { result = false; return; }
				png = [rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
				if (!png) { result = false; return; }
			}
			NSUInteger len = [png length];
			if (len == 0) return;
			data.AllocMemory((unsigned long)len);
			memcpy(data.data(), [png bytes], len);
		}
	});
	return result;
}

bool BaseHelper::ClipboardManager::SetImage(VH& data, bool bEmpty)
{
	__block bool result = false;
	runOnMainSync(^{
		@autoreleasepool {
			NSData* png = [NSData dataWithBytes:data.data() length:data.size()];
			if (!png) return;
			NSPasteboard* pb = [NSPasteboard generalPasteboard];
			if (!pb) return;
			if (bEmpty) [pb clearContents];
			// PNG — основной формат. TIFF дописываем для совместимости со стандартными приложениями macOS,
			// но только если PNG-запись успешна — иначе пастборд может быть закрыт другим приложением.
			BOOL pngOk = [pb setData:png forType:NSPasteboardTypePNG];
			if (!pngOk) return;
			NSBitmapImageRep* rep = [NSBitmapImageRep imageRepWithData:png];
			if (rep) {
				NSData* tiff = [rep TIFFRepresentation];
				if (tiff) [pb setData:tiff forType:NSPasteboardTypeTIFF];
			}
			result = true;
		}
	});
	return result;
}

bool BaseHelper::ClipboardManager::Empty()
{
	@autoreleasepool {
		[[NSPasteboard generalPasteboard] clearContents];
	}
	return true;
}

#endif //__APPLE__
