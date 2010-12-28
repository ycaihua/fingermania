//
//  FingerManiaAppDelegate.h
//  FingerMania
//
//  Created by 李 杰军 on 10-12-28.
//  Copyright 2010 Virtuos. All rights reserved.
//

#import <UIKit/UIKit.h>

@class FingerManiaViewController;

@interface FingerManiaAppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow *window;
    FingerManiaViewController *viewController;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet FingerManiaViewController *viewController;

@end

