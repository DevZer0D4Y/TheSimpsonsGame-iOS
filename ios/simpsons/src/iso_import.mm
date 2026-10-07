#include "iso_import.h"
#include <rex/filesystem/devices/disc_image_device.h>
#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

@interface SimpsonsISOImporter : NSObject <UIDocumentPickerDelegate>
@property(nonatomic, strong) UIViewController* presenter;
- (void)offerImport;
@end

@implementation SimpsonsISOImporter {
  std::function<void(std::filesystem::path)> ready_;
}
- (instancetype)initWithReady:(std::function<void(std::filesystem::path)>)ready {
  if ((self = [super init])) ready_ = std::move(ready);
  return self;
}
- (void)offerImport {
  UIAlertController* alert = [UIAlertController alertControllerWithTitle:@"Load The Simpsons Game"
      message:@"Select your Xbox 360 ISO/XISO, or copy extracted game files into the game folder in Files. Saves are kept separately."
      preferredStyle:UIAlertControllerStyleAlert];
  [alert addAction:[UIAlertAction actionWithTitle:@"Choose ISO" style:UIAlertActionStyleDefault handler:^(UIAlertAction* action) {
    UIDocumentPickerViewController* picker = [[UIDocumentPickerViewController alloc]
        initForOpeningContentTypes:@[UTTypeData] asCopy:NO];
    picker.delegate = self;
    picker.allowsMultipleSelection = NO;
    [self.presenter presentViewController:picker animated:YES completion:nil];
  }]];
  [self.presenter presentViewController:alert animated:YES completion:nil];
}
- (void)documentPickerWasCancelled:(UIDocumentPickerViewController*)controller {
  [controller dismissViewControllerAnimated:YES completion:^{ [self offerImport]; }];
}
- (void)documentPicker:(UIDocumentPickerViewController*)controller didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls {
  NSURL* source = urls.firstObject;
  if (!source) { [self offerImport]; return; }
  BOOL scoped = [source startAccessingSecurityScopedResource];
  UIAlertController* progress = [UIAlertController alertControllerWithTitle:@"Importing ISO"
      message:@"Keep the app open while the image is copied. Large disc images can take several minutes."
      preferredStyle:UIAlertControllerStyleAlert];
  [controller dismissViewControllerAnimated:YES completion:^{
    [self.presenter presentViewController:progress animated:YES completion:nil];
  }];
  dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
    NSURL* docs = [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] firstObject];
    NSURL* stage = [docs URLByAppendingPathComponent:@"game.importing"];
    NSURL* target = [docs URLByAppendingPathComponent:@"game.iso"];
    NSFileManager* fm = NSFileManager.defaultManager;
    [fm removeItemAtURL:stage error:nil];
    NSError* error = nil;
    __block BOOL copied = NO;
    NSFileCoordinator* coordinator = [[NSFileCoordinator alloc] initWithFilePresenter:nil];
    __block NSError* copyError = nil;
    [coordinator coordinateReadingItemAtURL:source options:0 error:&error byAccessor:^(NSURL* readable) {
      copied = [fm copyItemAtURL:readable toURL:stage error:&copyError];
    }];
    if (!error) error = copyError;
    if (copied && !error) {
      rex::filesystem::DiscImageDevice disc("validate", std::filesystem::path(stage.path.UTF8String));
      if (!disc.Initialize() || !disc.ResolvePath("default.xex")) {
        copied = NO;
        error = [NSError errorWithDomain:@"SimpsonsISO" code:1 userInfo:@{
          NSLocalizedDescriptionKey: @"Choose a valid Xbox 360 ISO/XISO containing default.xex."}];
      }
    }
    if (copied && !error) {
      if ([fm fileExistsAtPath:target.path]) {
        copied = [fm replaceItemAtURL:target withItemAtURL:stage backupItemName:nil options:0 resultingItemURL:nil error:&error];
      } else {
        copied = [fm moveItemAtURL:stage toURL:target error:&error];
      }
    }
    if (scoped) [source stopAccessingSecurityScopedResource];
    if (!copied) [fm removeItemAtURL:stage error:nil];
    dispatch_async(dispatch_get_main_queue(), ^{
      [progress dismissViewControllerAnimated:YES completion:^{
        if (copied && !error) {
          ready_(std::filesystem::path(target.path.UTF8String));
        } else {
          UIAlertController* failure = [UIAlertController alertControllerWithTitle:@"Could not import ISO"
              message:error.localizedDescription ?: @"Check available storage and try again."
              preferredStyle:UIAlertControllerStyleAlert];
          [failure addAction:[UIAlertAction actionWithTitle:@"Try again" style:UIAlertActionStyleDefault handler:^(UIAlertAction* action) { [self offerImport]; }]];
          [self.presenter presentViewController:failure animated:YES completion:nil];
        }
      }];
    });
  });
}
@end

void SimpsonsImportISO(std::function<void(std::filesystem::path)> ready) {
  // Retained while a provider asynchronously supplies the selected document.
  static SimpsonsISOImporter* importer;
  importer = [[SimpsonsISOImporter alloc] initWithReady:std::move(ready)];
  for (UIScene* scene in UIApplication.sharedApplication.connectedScenes) {
    if (![scene isKindOfClass:UIWindowScene.class]) continue;
    for (UIWindow* window in ((UIWindowScene*)scene).windows) {
      if (window.isKeyWindow) { importer.presenter = window.rootViewController; break; }
    }
    if (importer.presenter) break;
  }
  [importer offerImport];
}
