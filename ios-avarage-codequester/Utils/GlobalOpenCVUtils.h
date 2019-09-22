//
//  GlobalOpenCVUtils.h
//  ios-avarage-codequester
//
//  Created by HD on 03/09/2019.
//  Copyright © 2019 codequest. All rights reserved.
//

#ifndef GlobalOpenCVUtils_h
#define GlobalOpenCVUtils_h

#import <opencv2/core.hpp>

bool cmpArea(const cv::Rect & lhs, const cv::Rect & rhs) {
    return lhs.area() > rhs.area();
}

cv::Rect operator* (const cv::Rect & rect, int scale) {
    return cv::Rect(rect.x * scale, rect.y * scale, rect.width * scale, rect.height * scale);
}


#endif /* GlobalOpenCVUtils_h */
