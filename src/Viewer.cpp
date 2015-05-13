#include <opencv2/core.hpp>

#include <pcl/io/pcd_io.h>

#include <boost/date_time/posix_time/posix_time.hpp>
namespace ptime = boost::posix_time;

#include <boost/format.hpp>
typedef boost::format fmt;

#include "Viewer.hpp"

Viewer::Viewer()
:
	_log("PCL"),
	_viewer("Point Cloud")
{
	// Set viewer settings
	_viewer.setShowFPS(true);
	_viewer.setWindowBorders(false);

	_viewer.setBackgroundColor(0.1, 0.1, 0.1);
	_viewer.addCoordinateSystem(0.1, "global");
}

void Viewer::reduceCloud(cloud_t::Ptr& cloud)
{
	const float voxel_resolution = 2.f;

	// Configure grid
	_grid.setInputCloud(cloud);
	_grid.setLeafSize(voxel_resolution, voxel_resolution, voxel_resolution);

	// Filter
	cloud_t::Ptr reduc(new cloud_t);
	_grid.filter(*reduc);

	std::swap(cloud, reduc);
}

void Viewer::showCloudPoints(const Images& images, const CameraPoses& poses,
	const cv::Mat& cameraMatrix)
{
	// Fill cloud structure
	cloud_t::Ptr pcl_points(new cloud_t);

	// Per camera
	for (int c = 0; c < poses.size(); c++)
	{
		Image   image = images[c];
		cv::Mat R     = poses[c].R;
		cv::Mat t     = poses[c].t;
		if (R.empty()) continue;

		// Per pixel
		const cv::Vec3b* rgbs;
		const float*     deps;

		cv::Vec3b rgb;
		float     dep;

		cv::Point3f point;
		cv::Mat     gPoint;

		for (int i = 0; i < image.dep.rows; i++)
		{
			rgbs = image.rgb.ptr<cv::Vec3b>(i);
			deps = image.dep.ptr<float>(i);
			for (int j = 0; j < image.dep.cols; j++)
			{
				rgb = rgbs[j];
				dep = deps[j];

				// Valid depth is between 40cm and 8m
				if (dep < 400 || dep > 8000) continue;

				// Calculate point pos in global coordinates
				point  = backproject3D(j, i, dep, cameraMatrix);
				gPoint = R.t() * cv::Mat(point) - t;
				point  = cv::Point3f(gPoint);

				point_t pcl_p;
				pcl_p.x = point.x;
				pcl_p.y = point.y;
				pcl_p.z = point.z;
				pcl_p.r = rgb[2];
				pcl_p.g = rgb[1];
				pcl_p.b = rgb[0];
				pcl_points->points.push_back(pcl_p);
			}
		}
		// Reduce points every 10 cameras
		if (c % 10 == 0) reduceCloud(pcl_points);
	}

	// Final reduction of points
	reduceCloud(pcl_points);

	// TODO: Save to disk with timestamp
	const int n = pcl_points->points.size();
	auto time   = ptime::second_clock::local_time();
	auto tstamp = ptime::to_iso_string(time);
	auto fname  = (fmt("output_%s_%d.pcd") % tstamp % n).str();
	pcl::PCDWriter writer;
	writer.writeBinaryCompressed(fname, *pcl_points);

	// Show cloud
	_log("\nShowing %d points", n);
	vis::PointCloudColorHandlerRGBField<point_t> rgb_handler(pcl_points);
	_viewer.addPointCloud<pcl::PointXYZRGB>(pcl_points, rgb_handler);
	_log.tok();

	// Wait until closed
	while (!_viewer.wasStopped())
	{
		_viewer.spinOnce(15);
	}
}

