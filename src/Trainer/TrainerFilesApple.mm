// SPDX-License-Identifier: LGPL-3.0-or-later
#ifdef PVZ_TRAINER
#include "Common.h"
#include "TrainerFiles.h"
#import <Foundation/Foundation.h>
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

namespace {
UIViewController *Presenter() {
  UIViewController *controller = nil;
  for (UIScene *scene in UIApplication.sharedApplication.connectedScenes)
    if ([scene isKindOfClass:UIWindowScene.class] &&
        scene.activationState == UISceneActivationStateForegroundActive)
      for (UIWindow *window in ((UIWindowScene *)scene).windows)
        if (window.isKeyWindow)
          controller = window.rootViewController;
  while (controller.presentedViewController)
    controller = controller.presentedViewController;
  if (controller.isBeingDismissed || controller.isBeingPresented)
    return nil;
  return controller;
}
NSString *AppDirectory(const char *relative) {
  std::string path = Sexy::GetAppDataPath(relative);
  return [[NSString alloc] initWithBytes:path.data()
                                  length:path.size()
                                encoding:NSUTF8StringEncoding];
}
BOOL BoundedImport(NSURL *root) {
  NSArray *keys =
      @[ NSURLIsDirectoryKey, NSURLIsSymbolicLinkKey, NSURLFileSizeKey ];
  NSDictionary *first = [root resourceValuesForKeys:keys error:nil];
  if (!first || [first[NSURLIsSymbolicLinkKey] boolValue])
    return NO;
  unsigned long long bytes = [first[NSURLFileSizeKey] unsignedLongLongValue];
  if (![first[NSURLIsDirectoryKey] boolValue])
    return bytes <= 512ULL * 1024 * 1024;
  NSDirectoryEnumerator *files =
      [NSFileManager.defaultManager enumeratorAtURL:root
                         includingPropertiesForKeys:keys
                                            options:0
                                       errorHandler:nil];
  if (!files)
    return NO;
  unsigned count = 0;
  for (NSURL *url in files) {
    if (++count > 10000)
      return NO;
    NSDictionary *values = [url resourceValuesForKeys:keys error:nil];
    if (!values || [values[NSURLIsSymbolicLinkKey] boolValue])
      return NO;
    if (![values[NSURLIsDirectoryKey] boolValue])
      bytes += [values[NSURLFileSizeKey] unsignedLongLongValue];
    if (bytes > 512ULL * 1024 * 1024)
      return NO;
  }
  return YES;
}
} // namespace

@interface PvZTrainerDocumentDelegate : NSObject <UIDocumentPickerDelegate>
@property(nonatomic) uint64_t ticket;
@end
static PvZTrainerDocumentDelegate *documentDelegate;
@implementation PvZTrainerDocumentDelegate
- (void)documentPickerWasCancelled:
    (UIDocumentPickerViewController *)controller {
  Trainer::FilesDetail::Complete(self.ticket, {});
  documentDelegate = nil;
}
- (void)documentPicker:(UIDocumentPickerViewController *)controller
    didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls {
  if (urls.count == 0) {
    Trainer::FilesDetail::Complete(self.ticket, {});
    documentDelegate = nil;
    return;
  }
  NSURL *source = urls.firstObject;
  uint64_t ticket = self.ticket;
  BOOL scoped = [source startAccessingSecurityScopedResource];
  dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
    @autoreleasepool {
      NSFileManager *fm = NSFileManager.defaultManager;
      NSString *base = [AppDirectory("trainer/imports")
          stringByAppendingPathComponent:NSUUID.UUID.UUIDString];
      NSError *error = nil;
      [fm createDirectoryAtPath:base
          withIntermediateDirectories:YES
                           attributes:nil
                                error:&error];
      NSString *filename = source.lastPathComponent.length
                               ? source.lastPathComponent
                               : @"import";
      NSURL *destination = [NSURL
          fileURLWithPath:[base stringByAppendingPathComponent:filename]];
      NSFileCoordinator *coordinator =
          [[NSFileCoordinator alloc] initWithFilePresenter:nil];
      __block BOOL copied = NO;
      [coordinator coordinateReadingItemAtURL:source
                                      options:0
                                        error:&error
                                   byAccessor:^(NSURL *readable) {
                                     if (BoundedImport(readable))
                                       copied = [fm copyItemAtURL:readable
                                                            toURL:destination
                                                            error:nil];
                                   }];
      if (!copied)
        [fm removeItemAtPath:base error:nil];
      if (scoped)
        [source stopAccessingSecurityScopedResource];
      Trainer::FilesDetail::Complete(
          ticket,
          copied ? std::string(destination.path.UTF8String) : std::string{});
    }
  });
  documentDelegate = nil;
}
@end

namespace Trainer::FilesDetail {
bool Available() { return true; }
void Choose(std::uint64_t ticket, int kind) {
  dispatch_async(dispatch_get_main_queue(), ^{
    @autoreleasepool {
      if (kind == 1) {
        NSString *directory = AppDirectory("trainer/exports");
        NSError *error = nil;
        if (![NSFileManager.defaultManager createDirectoryAtPath:directory
                                     withIntermediateDirectories:YES
                                                      attributes:nil
                                                           error:&error]) {
          Complete(ticket, {});
          return;
        }
        NSString *path =
            [directory stringByAppendingPathComponent:
                           [NSString stringWithFormat:@"toolkit-%@.txt",
                                                      NSUUID.UUID.UUIDString]];
        Complete(ticket, std::string(path.UTF8String));
        return;
      }
      UIViewController *presenter = Presenter();
      if (!presenter || documentDelegate) {
        Complete(ticket, {});
        return;
      }
      UIDocumentPickerViewController *picker =
          [[UIDocumentPickerViewController alloc]
              initForOpeningContentTypes:@[ kind == 2 ? UTTypeFolder
                                                      : UTTypeItem ]
                                  asCopy:kind != 2];
      documentDelegate = [PvZTrainerDocumentDelegate new];
      documentDelegate.ticket = ticket;
      picker.delegate = documentDelegate;
      picker.allowsMultipleSelection = NO;
      [presenter presentViewController:picker animated:YES completion:nil];
    }
  });
}
bool Share(const std::string &path) {
  NSString *value = [[NSString alloc] initWithBytes:path.data()
                                             length:path.size()
                                           encoding:NSUTF8StringEncoding];
  if (!value)
    return false;
  dispatch_async(dispatch_get_main_queue(), ^{
    UIViewController *presenter = Presenter();
    if (!presenter)
      return;
    UIActivityViewController *share = [[UIActivityViewController alloc]
        initWithActivityItems:@[ [NSURL fileURLWithPath:value] ]
        applicationActivities:nil];
    UIPopoverPresentationController *popover =
        share.popoverPresentationController;
    if (popover) {
      popover.sourceView = presenter.view;
      popover.sourceRect =
          CGRectMake(CGRectGetMidX(presenter.view.bounds),
                     CGRectGetMidY(presenter.view.bounds), 1, 1);
      popover.permittedArrowDirections = 0;
    }
    [presenter presentViewController:share animated:YES completion:nil];
  });
  return true;
}
} // namespace Trainer::FilesDetail
#else
#import <AppKit/AppKit.h>
namespace Trainer::FilesDetail {
bool Available() { return true; }
void Choose(std::uint64_t ticket, int kind) {
  dispatch_async(dispatch_get_main_queue(), ^{
    @autoreleasepool {
      NSSavePanel *panel;
      if (kind == 1) {
        panel = [NSSavePanel savePanel];
        panel.nameFieldStringValue = @"toolkit-export.txt";
        panel.canCreateDirectories = YES;
      } else {
        NSOpenPanel *open = [NSOpenPanel openPanel];
        open.canChooseDirectories = kind == 2;
        open.canChooseFiles = kind != 2;
        open.allowsMultipleSelection = NO;
        panel = open;
      }
      panel.title = @"PvZ Toolkit";
      [panel beginWithCompletionHandler:^(NSModalResponse response) {
        Complete(ticket, response == NSModalResponseOK && panel.URL
                             ? std::string(panel.URL.path.UTF8String)
                             : std::string{});
      }];
    }
  });
}
bool Share(const std::string &path) {
  NSString *value = [[NSString alloc] initWithBytes:path.data()
                                             length:path.size()
                                           encoding:NSUTF8StringEncoding];
  if (!value)
    return false;
  dispatch_async(dispatch_get_main_queue(), ^{
    [NSWorkspace.sharedWorkspace
        activateFileViewerSelectingURLs:@[ [NSURL fileURLWithPath:value] ]];
  });
  return true;
}
} // namespace Trainer::FilesDetail
#endif
#endif
