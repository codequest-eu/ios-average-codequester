//
//  MatSaver.h
//  ios-avarage-codequester
//
//  Created by HD on 03/09/2019.
//  Copyright © 2019 codequest. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <opencv2/core.hpp>

NS_ASSUME_NONNULL_BEGIN

@interface MatSaver : NSObject

+ (void) saveMat: (cv::Mat) mat inFile: (NSString *) filename;

@end

NS_ASSUME_NONNULL_END
