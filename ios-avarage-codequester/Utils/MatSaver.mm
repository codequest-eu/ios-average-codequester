//
//  MatSaver.m
//  ios-avarage-codequester
//
//  Created by HD on 03/09/2019.
//  Copyright © 2019 codequest. All rights reserved.
//

#import "MatSaver.h"
#import <opencv2/core.hpp>
#import <opencv2/imgcodecs.hpp>
#import <opencv2/imgcodecs/ios.h>

@implementation MatSaver

+ (NSString *) pathForFilename: (NSString *) filename {
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    return [NSString stringWithFormat: @"%@/%@", paths[0], filename];
}

+ (void) saveMat: (cv::Mat) mat inFile: (NSString *) filename {
    NSString *file = [self pathForFilename:filename];
    NSLog(@"%@", file);
    imwrite([file UTF8String], mat);
}

@end
