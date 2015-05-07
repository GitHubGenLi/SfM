#include "opencv2/opencv.hpp"
using namespace cv;

#include "Pipeline.hpp"
#include "util.hpp"

float _intrinsic_array[9]= {524,0,316.7,0,524,238.5,0,0,1};
float _distcoeff_array[4] = {0.2402,-0.6861,-0.0015,0.0003};

Pipeline::Pipeline(std::string _folder_path)
:
	cameraMatrix(Mat(3, 3, CV_32F, _intrinsic_array)),
	distCoeffs(Mat(1, 4, CV_32F, _distcoeff_array))
{
	folder_path = _folder_path;
}

void Pipeline::run()
{
	Logger _log("Pipeline");
	/**
	 * Stage 0: Load images from file
	 */
	Images images;
	load_images(folder_path, images);


	/**
	 * Stage 1: Detect features in loaded images
	 */
	CamFrames cam_Frames;
	DescriptorsVec descriptors_vec;
	extract_features(images, cam_Frames, descriptors_vec);


	/**
	 * Stage 2: Calculate descriptors and find image pairs through matching
	 */
	ImagePairs image_pairs;
	find_matching_pairs(images, cam_Frames, descriptors_vec, image_pairs);
	// free (&images); or actually free everything in images except rgb

	/**
	* State 3: Compute pairwise R and t
	*/
	register_camera(image_pairs,cam_Frames);


	/**
	 * Stage 3.5: Construct associative matrix
	 */
	Associativity assocMat(cam_Frames.size());
	for (int p = 0; p < image_pairs.size(); p++)
	{
		ImagePair* pair = &image_pairs[p];
		int i = pair->pair_index.first,
		    j = pair->pair_index.second;
		assocMat(i, j) = pair;
		assocMat(j, i) = pair;

		assert(assocMat(i, j) == assocMat(j, i));
		assert(assocMat(i, j)->pair_index.first  == i &&
		       assocMat(i, j)->pair_index.second == j);
	}



	/**
	 * Stage 4: Compute global Rs and ts
	 */
	CameraPoses gCameraPoses;
	build_spanning_tree(gCameraPoses, image_pairs, assocMat);

	_log.tok();
}
