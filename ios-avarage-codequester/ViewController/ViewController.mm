//
//  ViewController.m
//  ios-avarage-codequester
//
//  Created by HD on 05/04/2019.
//  Copyright © 2019 codequest. All rights reserved.
//

#import "ViewController.h"

#import <opencv2/core.hpp>
#import <opencv2/imgcodecs.hpp>
#import <opencv2/imgcodecs/ios.h>
#import <opencv2/imgproc.hpp>
#import <opencv2/objdetect.hpp>
#import <opencv2/face.hpp>

#import <stdio.h>

#import "faceBlendCommon.hpp"
#import "MatSaver.h"
#import "GlobalOpenCVUtils.h"

using namespace std;
using namespace cv;
using namespace cv::face;

@interface ViewController () {
    CascadeClassifier faceClassifier;
    Ptr<Facemark> facemark;

    // vectors used by the whole algorithm, have data about all images
    vector<vector<Point2f>> allFaceLandmarks; // vector of vectors of points for all face landmarks
    vector<vector<Point2f>> allFaceLandmarkPointsNormalized;
    vector<Mat> allFaceImages;
    vector<Mat> allFaceImagesNormalized;

    // vector used for one image, but initialized one time for better performance
    vector<cv::Rect> oneImageFaces;
    vector<cv::Rect> oneImageBiggestFace; // just one face, but vector is needed for the method
    vector<vector<Point2f>> oneFaceLandmarkPoints;

    // like above - Mats used for one face, but reused in algorithm
    Mat faceMat;
    Mat bgrFaceMat;
    Mat smallerMat;

    vector<Point2f> boundaryPts; // 8 Boundary points for Delaunay Triangulation
}

@property (weak, nonatomic) IBOutlet UIButton * avarageFaceButton;
@property (weak, nonatomic) IBOutlet UIImageView * imageView;

@end

static int imagesCounter = 29;
static cv::Size normalizedSize(600, 600); // Dimensions of output image

@implementation ViewController

- (void) viewDidLoad {
    [super viewDidLoad];
    [self initFaceClassifier];
}

- (UIStatusBarStyle) preferredStatusBarStyle {
    return UIStatusBarStyleLightContent;
}

- (IBAction) generateAvarageFaceTapped: (id) sender {
    [self.avarageFaceButton setHidden: YES];

    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    dispatch_async(queue, ^{
        NSArray * allFiles = [self prepareFaceImageNames];
        UIImage * processed = [self generateAvarageFaceFrom: allFiles];
        dispatch_async(dispatch_get_main_queue(), ^{
            self.imageView.image = processed;
            [self.avarageFaceButton setHidden: NO];
        });
    });
}

- (void) initFaceClassifier {
    NSString *faceCascadeName = [[NSBundle mainBundle] pathForResource:@"haarcascade_frontalface_alt2" ofType:@"xml"];
    NSString *facemarkModel = [[NSBundle mainBundle] pathForResource:@"lbfmodel" ofType:@"yaml"];

    faceClassifier.load(string([faceCascadeName UTF8String]));

    facemark = Ptr<Facemark>(FacemarkLBF::create());
    facemark->loadModel(string([facemarkModel UTF8String]));
}

- (NSArray *) prepareFaceImageNames {
    NSMutableArray * allFiles = [[NSMutableArray alloc] init];
    for (int i = 0; i < imagesCounter; ++i) {
        [allFiles addObject: [NSString stringWithFormat:@"codequest_%d.jpg", i]];
    }
    return allFiles;
}

- (UIImage *) generateAvarageFaceFrom: (NSArray *) facesFiles {
    NSDate * methodStart = [NSDate date];
    NSLog(@"Start");

    allFaceLandmarks.clear();
    allFaceImages.clear();

    for (int i = 0; i < [facesFiles count]; ++i) {
        NSString * faceFileName = facesFiles[i];
        UIImage * faceImage = [UIImage imageNamed: faceFileName];
        UIImageToMat(faceImage, faceMat);
        cvtColor(faceMat, bgrFaceMat, COLOR_RGBA2BGR);

        [self findLandmarksOn: bgrFaceMat imageNumber: i];

        NSLog(@"processed: %@", faceFileName);
    }

    Mat avarageOne = [self generateAvarageFaceFromFoundLandmarks];
    [MatSaver saveMat: avarageOne inFile: @"avarage.jpg"];

    cvtColor(avarageOne, avarageOne, COLOR_BGR2RGB);
    UIImage * avarageImage = MatToUIImage(avarageOne);

    // Log execution time
    NSDate *methodFinish = [NSDate date];
    NSTimeInterval executionTime = [methodFinish timeIntervalSinceDate: methodStart];
    NSLog(@"executionTime = %f", executionTime);

    return avarageImage;
}

- (void) findLandmarksOn: (Mat &) faceMat imageNumber: (int) number {
    oneImageFaces.clear();
    oneImageBiggestFace.clear();
    oneFaceLandmarkPoints.clear();

    [self detectFaceRegionOn: faceMat imageNumber: number];

    if (facemark->fit(faceMat, oneImageBiggestFace, oneFaceLandmarkPoints)) {
        Mat converted;
        faceMat.convertTo(converted, CV_32FC3, 1/255.0);

        allFaceLandmarks.push_back(oneFaceLandmarkPoints[0]);
        allFaceImages.push_back(converted);

        NSLog(@"Found landmarks: %lu", oneFaceLandmarkPoints[0].size());

        // drawing and saving
        for(unsigned long i = 0; i < oneFaceLandmarkPoints[0].size(); i++) {
            cv::circle(faceMat, oneFaceLandmarkPoints[0][i], 3, cv::Scalar(255,0,0), FILLED);
        }
        [MatSaver saveMat: faceMat inFile: [NSString stringWithFormat: @"landmarks_%d.jpg", number]];
    }
}

- (void) detectFaceRegionOn: (Mat &) faceMat imageNumber: (int) number {
    Mat imageFrameGray;
    cvtColor(faceMat, imageFrameGray, COLOR_BGR2GRAY);
    equalizeHist(imageFrameGray, imageFrameGray);

    faceClassifier.detectMultiScale(imageFrameGray, oneImageFaces, 1.1, 3, 0, cv::Size(30, 30));

    if (oneImageFaces.size() == 0) {
        return;
    }

    // choose biggest face - only one
    sort(oneImageFaces.begin(), oneImageFaces.end(), cmpArea);
    oneImageBiggestFace.push_back(oneImageFaces[0]);

    // drawing and saving
    rectangle(imageFrameGray, oneImageFaces[0], Scalar(255, 0, 0));
    [MatSaver saveMat: imageFrameGray inFile: [NSString stringWithFormat: @"face_%d.jpg", number]];
}

- (Mat) generateAvarageFaceFromFoundLandmarks {
    unsigned long numImages = allFaceImages.size();

    // Space for normalized images and points
    allFaceImagesNormalized.clear();
    allFaceLandmarkPointsNormalized.clear();
    boundaryPts.clear();

    // Space for average landmark points
    vector <Point2f> averageFaceLandmarks(allFaceLandmarks[0].size());

    getEightBoundaryPoints(normalizedSize, boundaryPts);

    // Warp images and transform landmarks to output coordinate system
    // and find average of transformed landmarks
    for(int i = 0; i < allFaceImages.size(); i++) {
        vector<Point2f> points = allFaceLandmarks[i];

        Mat img;
        normalizeImagesAndLandmarks(normalizedSize, allFaceImages[i], img, points, points);

        // Calculate average landmark locations
        for (int j = 0; j < points.size(); j++) {
            averageFaceLandmarks[j] += points[j] * ( 1.0 / numImages);
        }

        // Append boundary points. Will be used in Delaunay Triangulation
        [self appendBundaryPointsTo: points];

        allFaceLandmarkPointsNormalized.push_back(points);
        allFaceImagesNormalized.push_back(img);

        Mat imageF_8UC3;
        img.convertTo(imageF_8UC3, CV_8UC3, 255);
        [MatSaver saveMat: imageF_8UC3 inFile: [NSString stringWithFormat: @"normalized_%d.jpg", i]];
    }

    // Append boundary points to average points
    [self appendBundaryPointsTo: averageFaceLandmarks];

    // Calculate Delaunay triangles for average landmarks
    cv::Rect rect(0, 0, normalizedSize.width, normalizedSize.height);
    vector<vector<int>> dt;
    calculateDelaunayTriangles(rect, averageFaceLandmarks, dt);

    // Draw avarage Delaunay
    Mat avDelaunay = Mat::zeros(normalizedSize, CV_8UC3);
    drawDelaunay(avDelaunay, averageFaceLandmarks, Scalar(255,255,255));
    [MatSaver saveMat: avDelaunay inFile: @"avarage_delaunay.jpg"];

    // Space for output image
    Mat output = Mat::zeros(normalizedSize, allFaceImagesNormalized[0].type()); // CV_32FC3

    // Warp input images to average image landmarks
    for(int i = 0; i < numImages; i++) {
        // Draw Delaunay before warp
        [self drawDelaunayOn: allFaceImagesNormalized[i]
                  withPoints: allFaceLandmarkPointsNormalized[i]
                      saveAs: @"delaunay_before"
                 imageNumber: i];

        Mat warped;
        warpImage(allFaceImagesNormalized[i], warped, allFaceLandmarkPointsNormalized[i], averageFaceLandmarks, dt);

        // Draw Delaunay after warp
        [self drawDelaunayOn: warped withPoints: averageFaceLandmarks saveAs: @"delaunay_after" imageNumber: i];

        // Add image intensities for averaging
        output = output + warped;

        // Save in between image
        Mat part = output / (double)i;
        Mat imageF_8UC3;
        part.convertTo(imageF_8UC3, CV_8UC3, 255);
        [MatSaver saveMat: imageF_8UC3 inFile: [NSString stringWithFormat: @"avarage_part_%d.jpg", i]];
    }

    // Divide by numImages to get average
    output = output / (double)numImages;

    Mat imageF_8UC3;
    output.convertTo(imageF_8UC3, CV_8UC3, 255);

    return imageF_8UC3;
}

- (void) appendBundaryPointsTo: (vector<Point2f> &) points {
    for (int j = 0; j < boundaryPts.size(); j++) {
        points.push_back(boundaryPts[j]);
    }
}

- (void) drawDelaunayOn: (Mat &) mat
             withPoints: (vector<Point2f> &) points
                 saveAs: (NSString *) name
            imageNumber: (int) number {
    Mat normalized;
    mat.convertTo(normalized, CV_8UC3, 255);
    drawDelaunay(normalized, points, Scalar(255,255,255));
    [MatSaver saveMat: normalized inFile: [NSString stringWithFormat: @"%@_%d.jpg", name, number]];
}

@end
