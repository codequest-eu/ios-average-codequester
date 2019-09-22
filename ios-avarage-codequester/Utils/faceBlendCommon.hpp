/*
 Copyright 2017 BIG VISION LLC ALL RIGHTS RESERVED

 This program is distributed WITHOUT ANY WARRANTY to the
 Plus and Premium membership students of the online course
 titled "Computer Visionfor Faces" by Satya Mallick for
 personal non-commercial use.

 Sharing this code is strictly prohibited without written
 permission from Big Vision LLC.

 For licensing and other inquiries, please email
 spmallick@bigvisionllc.com

 */

/*

 Modified by: Hanna Dutkiewicz, codequest, 03.09.2019
 Permission to use these methods for educational purposes was received from Satya Mallick via e-mail on 26.08.2019

 */

#ifndef BIGVISION_faceBlendCommon_HPP_
#define BIGVISION_faceBlendCommon_HPP_

#import <opencv2/core.hpp>
#import <opencv2/imgcodecs.hpp>
#import <opencv2/imgcodecs/ios.h>
#import <opencv2/imgproc.hpp>
#import <opencv2/objdetect.hpp>
#import <opencv2/video.hpp>
#import <opencv2/calib3d.hpp>

using namespace cv;
using namespace std;

#ifndef M_PI
    #define M_PI 3.14159
#endif


// Constrains points to be inside boundary
static void constrainPoint(Point2f &p, cv::Size sz) {
    p.x = min(max((double)p.x, 0.0), (double)(sz.width - 1));
    p.y = min(max((double)p.y, 0.0), (double)(sz.height - 1));
}

// Returns 8 points on the boundary of a rectangle
static void getEightBoundaryPoints(cv::Size size, vector<Point2f>& boundaryPts) {
    int h = size.height, w = size.width;
    boundaryPts.push_back(Point2f(0,0));
    boundaryPts.push_back(Point2f(w/2, 0));
    boundaryPts.push_back(Point2f(w-1,0));
    boundaryPts.push_back(Point2f(w-1, h/2));
    boundaryPts.push_back(Point2f(w-1, h-1));
    boundaryPts.push_back(Point2f(w/2, h-1));
    boundaryPts.push_back(Point2f(0, h-1));
    boundaryPts.push_back(Point2f(0, h/2));
}

// Compute similarity transform given two pairs of corresponding points.
// OpenCV requires 3 points for calculating similarity matrix.
// We are hallucinating the third point.
static void similarityTransform(std::vector<cv::Point2f>& inPoints, std::vector<cv::Point2f>& outPoints, cv::Mat &tform) {

    double s60 = sin(60 * M_PI / 180.0);
    double c60 = cos(60 * M_PI / 180.0);

    vector <Point2f> inPts = inPoints;
    vector <Point2f> outPts = outPoints;

    // Placeholder for the third point.
    inPts.push_back(cv::Point2f(0,0));
    outPts.push_back(cv::Point2f(0,0));

    // The third point is calculated so that the three points make an equilateral triangle
    inPts[2].x = c60 * (inPts[0].x - inPts[1].x) - s60 * (inPts[0].y - inPts[1].y) + inPts[1].x;
    inPts[2].y = s60 * (inPts[0].x - inPts[1].x) + c60 * (inPts[0].y - inPts[1].y) + inPts[1].y;

    outPts[2].x = c60 * (outPts[0].x - outPts[1].x) - s60 * (outPts[0].y - outPts[1].y) + outPts[1].x;
    outPts[2].y = s60 * (outPts[0].x - outPts[1].x) + c60 * (outPts[0].y - outPts[1].y) + outPts[1].y;

    // Now we can use estimateAffinePartial2D for calculating the similarity transform.
    tform = estimateAffinePartial2D(inPts,outPts);
}

// Normalizes a facial image to a standard size given by outSize.
// The normalization is done based on landmark points passed as pointsIn
// After the normalization the left corner of the left eye is at (0.3 * w, h/3)
// and the right corner of the right eye is at (0.7 * w, h / 3) where w and h
// are the width and height of outSize.
static void normalizeImagesAndLandmarks(cv::Size outSize, Mat &imgIn, Mat &imgOut, vector<Point2f>& pointsIn, vector<Point2f>& pointsOut) {

    int h = outSize.height;
    int w = outSize.width;

    vector<Point2f> eyecornerSrc;
    // Get the locations of the left corner of left eye
    eyecornerSrc.push_back(pointsIn[36]);
    // Get the locations of the right corner of right eye
    eyecornerSrc.push_back(pointsIn[45]);

    vector<Point2f> eyecornerDst;
    // Location of the left corner of left eye in normalized image.
    eyecornerDst.push_back(Point2f(0.3*w, h/3));
    // Location of the right corner of right eye in normalized image.
    eyecornerDst.push_back(Point2f(0.7*w, h/3));

    // Calculate similarity transform
    Mat tform;
    similarityTransform(eyecornerSrc, eyecornerDst, tform);

    // Apply similarity transform to input image
    imgOut = Mat::zeros(h, w, CV_32FC3);
    warpAffine(imgIn, imgOut, tform, imgOut.size());

    // Apply similarity transform to landmarks
    transform(pointsIn, pointsOut, tform);
}

// In a vector of points, find the index of point closest to input point.
static int findIndex(vector<Point2f>& points, Point2f &point) {
    int minIndex = 0;
    double minDistance = norm(points[0] - point);

    for(int i = 1; i < points.size(); i++) {
        double distance = norm(points[i] - point);
        if(distance < minDistance) {
            minIndex = i;
            minDistance = distance;
        }
    }

    return minIndex;
}

// Calculate Delaunay triangles for set of points
// Returns the vector of indices of 3 points for each triangle
static void calculateDelaunayTriangles(cv::Rect rect, vector<Point2f> &points, vector< vector<int> > &delaunayTri) {

    // Create an instance of Subdiv2D
    Subdiv2D subdiv(rect);

    // Insert points into subdiv
    for(vector<Point2f>::iterator it = points.begin(); it != points.end(); it++)
    subdiv.insert(*it);

    // Get Delaunay triangulation
    vector<Vec6f> triangleList;
    subdiv.getTriangleList(triangleList);

    // Variable to store a triangle (3 points)
    vector<Point2f> pt(3);

    // Variable to store a triangle as indices from list of points
    vector<int> ind(3);

    for(size_t i = 0; i < triangleList.size(); i++) {
        // The triangle returned by getTriangleList is
        // a list of 6 coordinates of the 3 points in
        // x1, y1, x2, y2, x3, y3 format.
        Vec6f t = triangleList[i];

        // Store triangle as a vector of three points
        pt[0] = Point2f(t[0], t[1]);
        pt[1] = Point2f(t[2], t[3]);
        pt[2] = Point2f(t[4], t[5]);

        if (rect.contains(pt[0]) && rect.contains(pt[1]) && rect.contains(pt[2])) {
            // Find the index of each vertex in the points list
            for(int j = 0; j < 3; j++) {
                ind[j] = findIndex(points, pt[j]);
            }
            // Store triangulation as a list of indices
            delaunayTri.push_back(ind);
        }
    }
}

// Warps and alpha blends triangular regions from img1 and img2 to img
static void warpTriangle(Mat &img1, Mat &img2, vector<Point2f> tri1, vector<Point2f> tri2) {
    // Find bounding rectangle for each triangle
    cv::Rect r1 = boundingRect(tri1);
    cv::Rect r2 = boundingRect(tri2);

    // Crop the input image to the bounding box of input triangle
    Mat img1Cropped;
    img1(r1).copyTo(img1Cropped);

    // Offset points by left top corner of the respective rectangles
    vector<Point2f> tri1Cropped, tri2Cropped;
    vector<cv::Point> tri2CroppedInt;
    for(int i = 0; i < 3; i++) {
        tri1Cropped.push_back(Point2f(tri1[i].x - r1.x, tri1[i].y - r1.y));
        tri2Cropped.push_back(Point2f(tri2[i].x - r2.x, tri2[i].y - r2.y));

        // fillConvexPoly needs a vector of Point and not Point2f
        tri2CroppedInt.push_back(cv::Point((int)(tri2[i].x - r2.x), (int)(tri2[i].y - r2.y)));
    }

    // Given a pair of triangles, find the affine transform.
    Mat warpMat = getAffineTransform(tri1Cropped, tri2Cropped);

    // Apply the Affine Transform just found to the src image
    Mat img2Cropped = Mat::zeros(r2.height, r2.width, img1Cropped.type());
    warpAffine(img1Cropped, img2Cropped, warpMat, img2Cropped.size(), INTER_LINEAR, BORDER_REFLECT_101);

    // Get mask by filling triangle
    Mat mask = Mat::zeros(r2.height, r2.width, img2.type());
    fillConvexPoly(mask, tri2CroppedInt, Scalar(1.0, 1.0, 1.0), 16, 0);

    // Copy triangular region of the rectangular patch to the output image
    cv::multiply(img2Cropped, mask, img2Cropped);
    cv::multiply(img2(r2), Scalar(1.0,1.0,1.0) - mask, img2(r2));
    img2(r2) = img2(r2) + img2Cropped;
}

// Warps an image in a piecewise affine manner.
// The warp is defined by the movement of landmark points specified by pointsIn
// to a new location specified by pointsOut. The triangulation beween points is specified
// by their indices in delaunayTri.
static void warpImage(Mat &imgIn, Mat &imgOut, vector<Point2f> &pointsIn, vector<Point2f> &pointsOut, vector< vector<int> > &delaunayTri) {
    // Specify the output image the same size and type as the input image.
    cv::Size size = imgIn.size();
    imgOut = Mat::zeros(size, imgIn.type());

    // Warp each input triangle to output triangle.
    // The triangulation is specified by delaunayTri
    for(size_t j = 0; j < delaunayTri.size(); j++) {
        vector<Point2f> tin, tout;  // Input and output points corresponding to jth triangle

        for(int k = 0; k < 3; k++) {
            Point2f pIn = pointsIn[delaunayTri[j][k]]; // Extract a vertex of input triangle
            constrainPoint(pIn, size); // Make sure the vertex is inside the image

            Point2f pOut = pointsOut[delaunayTri[j][k]]; // Extract a vertex of the output triangle
            constrainPoint(pOut, size); // Make sure the vertex is inside the image.

            tin.push_back(pIn);  // Push the input vertex into input triangle
            tout.push_back(pOut); // Push the output vertex into output triangle
        }
        // Warp pixels inside input triangle to output triangle.
        warpTriangle(imgIn, imgOut, tin, tout);
    }
}

// Draw delaunay triangles
static void drawDelaunay(Mat& img, vector<Point2f> &points, Scalar delaunay_color) {

    cv::Size size = img.size();
    cv::Rect rect(0,0, size.width, size.height);

    // Create an instance of Subdiv2D
    Subdiv2D subdiv(rect);

    // Insert points into subdiv
    for(vector<Point2f>::iterator it = points.begin(); it != points.end(); it++)
    subdiv.insert(*it);

    vector<Vec6f> triangleList;
    subdiv.getTriangleList(triangleList);
    vector<cv::Point> pt(3);

    for(size_t i = 0; i < triangleList.size(); i++) {
        Vec6f t = triangleList[i];
        pt[0] = cv::Point(cvRound(t[0]), cvRound(t[1]));
        pt[1] = cv::Point(cvRound(t[2]), cvRound(t[3]));
        pt[2] = cv::Point(cvRound(t[4]), cvRound(t[5]));

        // Draw rectangles completely inside the image.
        if (rect.contains(pt[0]) && rect.contains(pt[1]) && rect.contains(pt[2])) {
            line(img, pt[0], pt[1], delaunay_color, 1, LINE_AA, 0);
            line(img, pt[1], pt[2], delaunay_color, 1, LINE_AA, 0);
            line(img, pt[2], pt[0], delaunay_color, 1, LINE_AA, 0);
        }
    }
}

#endif // BIGVISION_faceBlendCommon_HPP_
