// #include "vision_utils.h"

// using namespace cv;

// namespace vsu {

// void ArrowAxes::drawAxes(Mat& img) {
//     Point xPoint((int)(this->center.x + cos(this->angle)*this->length),
//                  (int)(this->center.y + sin(this->angle)*this->length));
//     Point yPoint((int)(this->center.x - sin(this->angle)*this->length*this->y_axis_ratio),
//                  (int)(this->center.y + cos(this->angle)*this->length*this->y_axis_ratio));
//     arrowedLine(img, center, xPoint, cl_x, 1, LINE_AA, 0, this->hooksize);
//     arrowedLine(img, center, yPoint, cl_y, 1, LINE_AA, 0, this->hooksize);
//     circle(img, center, 2, cl_center, 2);
// }

// void ArrowAxes::drawAxes(Mat& img, Point position, double angle, int length) {
//     Point xPoint((int)(position.x + cos(angle)*length),
//                  (int)(position.y + sin(angle)*length));
//     Point yPoint((int)(position.x - sin(angle)*length*this->y_axis_ratio),
//                  (int)(position.y + cos(angle)*length*this->y_axis_ratio));
//     arrowedLine(img, position, xPoint, cl_x, 1, LINE_AA, 0, this->hooksize);
//     arrowedLine(img, position, yPoint, cl_y, 1, LINE_AA, 0, this->hooksize);
//     circle(img, position, 2, cl_center, 2);
// }

// // inline std::string encode_string(QString &input) {
// //     return input.toUtf8().toHex().toStdString();
// // }

// // inline QString decode_string(std::string &input) {
// //     QByteArray bytes = QByteArray::fromHex(QString::fromStdString(input).toUtf8());
// //     return QString::fromUtf8(bytes);
// // }

// // bool imwrite_wstring(const std::wstring& filename, const cv::Mat& img, const std::vector<int>& params) {
// //     // Encode the image into a buffer
// //     std::vector<uchar> buffer;
// //     // Extract file extension to determine format (e.g., ".png", ".jpg")
// //     std::wstring extension = filename.substr(filename.find_last_of(L"."));
// //     std::string narrowExtension(extension.begin(), extension.end());

// //     if (!cv::imencode(narrowExtension, img, buffer, params)) {
// //         std::wcerr << L"Error: Could not encode image to buffer" << std::endl;
// //         return false;
// //     }

// //     // Write the buffer to a file with the wide string name in binary mode
// //     std::ofstream file(filename, std::ios::binary);
// //     if (!file.is_open()) {
// //         std::wcerr << L"Error: Could not open file for writing " << filename << std::endl;
// //         return false;
// //     }

// //     file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
// //     file.close();

// //     return true;
// // }

// // cv::Mat imread_wstring(const std::wstring& filename, int flags) {
// //     // Open the file with the wide string name in binary mode
// //     std::ifstream file(filename, std::ios::binary);
// //     if (!file.is_open()) {
// //         std::wcerr << L"Error: Could not open file " << filename << std::endl;
// //         return cv::Mat();
// //     }

// //     // Read the file data into a buffer
// //     std::vector<uchar> buffer(std::istreambuf_iterator<char>(file), {});
// //     file.close();

// //     // Decode the buffer using OpenCV
// //     return cv::imdecode(buffer, flags);
// // }

// // std::vector<cv::Point> tNeighborSimplify(const std::vector<cv::Point>& contour,
// //                                          int T, double tolerance) {
// //     const size_t N = contour.size();
// //     if (N < 2 || T < 2) return contour;               // Nothing to do

// //     std::vector<cv::Point> simplified;
// //     simplified.reserve(N);
// //     simplified.push_back(contour.front());            // Always keep the first point

// //     size_t i = 0;
// //     while (i < N - 1) {
// //         bool compressed = false;
// //         // j goes from far neighbor (i+T) back to the immediate next (i+1)
// //         const size_t j_max = std::min(i + static_cast<size_t>(T), N - 1);
// //         for (size_t j = j_max; j > i + 0; --j) {
// //             const cv::Point& p1 = simplified.back();  // start point
// //             const cv::Point& p2 = contour[j];         // candidate end point
// //             const cv::Point2f seg = p2 - p1;
// //             const double seg_len = cv::norm(seg);
// //             if (seg_len == 0.0) continue;             // degenerate; skip

// //             // Compute max distance of intermediate points k ∈ (i, j)
// //             double dmax = 0.0;
// //             for (size_t k = i + 1; k < j; ++k) {
// //                 const cv::Point& pk = contour[k];
// //                 // perpendicular distance from pk to line p1‑p2
// //                 double cross = std::abs(seg.x * (pk.y - p1.y) - seg.y * (pk.x - p1.x));
// //                 double dist = cross / seg_len;
// //                 if (dist > dmax) dmax = dist;
// //                 if (dmax > tolerance) break;          // early exit
// //             }

// //             if (dmax <= tolerance) {
// //                 // All intermediate points are close enough -> compress them
// //                 simplified.push_back(p2);
// //                 i = j;                                // jump forward
// //                 compressed = true;
// //                 break;
// //             }
// //         }

// //         if (!compressed) {
// //             // Could not compress with any neighbor → keep next point
// //             simplified.push_back(contour[i + 1]);
// //             ++i;
// //         }
// //     }

// //     return simplified;
// // }

// // void drawAxes2Img(Mat& img, Point position, double angle, int length, double hookSize,
// //                              Scalar xColour, Scalar yColour, Scalar centerColour) {
// //     const double yAxisRatio = 0.8;
// //     // create the axis
// //     Point xPoint((int)(position.x + cos(angle)*length), (int)(position.y + sin(angle)*length));
// //     Point yPoint((int)(position.x - sin(angle)*length*yAxisRatio), (int)(position.y + cos(angle)*length*yAxisRatio));
// //     arrowedLine(img, position, xPoint, xColour, 1, LINE_AA, 0, hookSize);
// //     arrowedLine(img, position, yPoint, yColour, 1, LINE_AA, 0, hookSize);
// //     circle(img, position, 2, centerColour, 2);
// // }

// // void computePcaOrientation(const std::vector<Point>& pts, double& angleOutput, Point2f& centerOutput) {
// //     int sz = static_cast<int>(pts.size());
// //     Mat data_pts = Mat(sz, 2, CV_64F);
// //     for (int i = 0; i < data_pts.rows; i++)
// //     {
// //         data_pts.at<double>(i, 0) = pts[i].x;
// //         data_pts.at<double>(i, 1) = pts[i].y;
// //     }
// //     // Perform PCA analysis
// //     PCA pca_analysis(data_pts, Mat(), PCA::DATA_AS_ROW);
// //     // Store the center of the object
// //     Point cntr = Point(static_cast<int>(pca_analysis.mean.at<double>(0, 0)),
// //                        static_cast<int>(pca_analysis.mean.at<double>(0, 1)));
// //     // Store the eigenvalues and eigenvectors
// //     std::vector<Point2d> eigen_vecs(2);
// //     std::vector<double> eigen_val(2);
// //     for (int i = 0; i < 2; i++)
// //     {
// //         eigen_vecs[i] = Point2d(pca_analysis.eigenvectors.at<double>(i, 0),
// //                                 pca_analysis.eigenvectors.at<double>(i, 1));
// //         eigen_val[i] = pca_analysis.eigenvalues.at<double>(i);
// //     }

// //     double angle = atan2(eigen_vecs[0].y, eigen_vecs[0].x); // orientation in radians
// //     angleOutput = angle;
// //     centerOutput = cntr;
// // }

// // double median(cv::Mat Input) {
// //     double m = (Input.rows*Input.cols) / 2;
// //     int bin = 0;
// //     double med = -1.0;

// //     int histSize = 256;
// //     float range[] = { 0, 256 };
// //     const float* histRange = { range };
// //     bool uniform = true;
// //     bool accumulate = false;
// //     cv::Mat hist;
// //     cv::calcHist( &Input, 1, 0, cv::Mat(), hist, 1, &histSize, &histRange, uniform, accumulate );

// //     for ( int i = 0; i < histSize && med < 0.0; ++i )
// //     {
// //         bin += cvRound( hist.at< float >( i ) );
// //         if ( bin > m && med < 0.0 )
// //             med = i;
// //     }

// //     return med;
// // }

// // Mat cropImageWithBorderOffset(Mat sourceImage, Rect boxBounding,int border) {
// //     Mat outputMat;
// //     int minOffset_X, maxOffset_X;
// //     int minOffset_Y, maxOffset_Y;

// //     Point topLeft = boxBounding.tl();
// //     Point botRight = boxBounding.br();

// //     minOffset_X = topLeft.x - border;
// //     maxOffset_X = botRight.x + border;

// //     minOffset_Y = topLeft.y - border;
// //     maxOffset_Y = botRight.y + border;

// //     if (minOffset_X < 0) {
// //         minOffset_X = 0;
// //     }

// //     if (maxOffset_X > sourceImage.cols) {
// //         maxOffset_X = sourceImage.cols;
// //     }

// //     if (minOffset_Y < 0) {
// //         minOffset_Y = 0;
// //     }

// //     if (maxOffset_Y > sourceImage.rows) {
// //         maxOffset_Y = sourceImage.rows;
// //     }

// //     outputMat = sourceImage(Range(minOffset_Y, maxOffset_Y), Range(minOffset_X, maxOffset_X));

// //     return outputMat;
// // }

// // void drawPickingBox(Mat& matSrc, RotatedRect rectRot, Scalar color) {
// //     cv::Point2f vertices2f[4];
// //     rectRot.points(vertices2f);

// //     cv::Point vertices[4];
// //     for(int i = 0; i < 4; ++i){
// //         vertices[i] = vertices2f[i];
// //     }

// //     line(matSrc, vertices[0], vertices[1], color, 1, LineTypes::LINE_AA);
// //     line(matSrc, vertices[1], vertices[2], color, 1, LineTypes::LINE_AA);
// //     line(matSrc, vertices[2], vertices[3], color, 1, LineTypes::LINE_AA);
// //     line(matSrc, vertices[3], vertices[0], color, 1, LineTypes::LINE_AA);
// // }

// }
