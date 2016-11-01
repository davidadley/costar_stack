#ifndef TABLE_SEGMENTER
#define TABLE_SEGMENTER

#include <pcl/io/pcd_io.h>
#include <pcl/filters/extract_indices.h>

// for pcl segmentation
#include <pcl/features/integral_image_normal.h>
#include <pcl/segmentation/organized_multi_plane_segmentation.h>

// for segment object above table
#include <pcl/surface/convex_hull.h>
#include <pcl/segmentation/extract_polygonal_prism_data.h>
#include <pcl/filters/project_inliers.h>

#include "sp_segmenter/utility/typedef.h"

void segmentCloudAboveTable(pcl::PointCloud<PointT>::Ptr &cloud_input, const pcl::PointCloud<PointT>::Ptr &convexHull, const double aboveTableMin = 0.01, const double aboveTableMax = 0.25)
{

	std::cerr << "\nSegment object above table \n";
	// Prism object.
	pcl::ExtractPolygonalPrismData<PointT> prism;
	prism.setInputCloud(cloud_input);
	prism.setInputPlanarHull(convexHull);

	// from 1 cm above table to 50 cm above table
	prism.setHeightLimits(aboveTableMin, aboveTableMax);
	pcl::PointIndices::Ptr objectIndices(new pcl::PointIndices);

	prism.segment(*objectIndices);
	// Get and show all points retrieved by the hull.
	pcl::PointCloud<PointT>::Ptr objects(new pcl::PointCloud<PointT>);
	pcl::ExtractIndices<PointT> extract;
	extract.setInputCloud(cloud_input);
	extract.setIndices(objectIndices);
	//extract.setKeepOrganized(true);
	extract.filter(*objects);
	*cloud_input = *objects;
}

pcl::PointCloud<pcl::PointXYZRGBA>::Ptr getTableConvexHull(pcl::PointCloud<pcl::PointXYZRGBA>::Ptr negative, 
	pcl::visualization::PCLVisualizer::Ptr viewer,
	double distanceThreshold = 0.02, double angularThreshold = 2.0, int minimalInliers = 5000)
{
	// Get Normal Cloud
	pcl::PointCloud<pcl::Normal>::Ptr normals (new pcl::PointCloud<pcl::Normal>);
	pcl::IntegralImageNormalEstimation<pcl::PointXYZRGBA, pcl::Normal> ne;
	ne.setNormalEstimationMethod (ne.AVERAGE_3D_GRADIENT);
	ne.setMaxDepthChangeFactor(0.02f);
	ne.setNormalSmoothingSize(10.0f);
	ne.setInputCloud(negative);
	ne.compute(*normals);

	// Segment planes
	pcl::OrganizedMultiPlaneSegmentation< pcl::PointXYZRGBA, pcl::Normal, pcl::Label > mps;

	mps.setMinInliers (minimalInliers);
	mps.setAngularThreshold (0.017453 * angularThreshold); // in radians
	mps.setDistanceThreshold (distanceThreshold); // in meters
	mps.setInputNormals (normals);
	mps.setInputCloud (negative);
	mps.setProjectPoints (true); // project the boundary points to the plane
	std::vector< pcl::PlanarRegion<pcl::PointXYZRGBA>, Eigen::aligned_allocator<pcl::PlanarRegion<pcl::PointXYZRGBA> > > regions;
	mps.segmentAndRefine (regions);
    
    // Retrieve the convex hull.
    pcl::PointCloud<pcl::PointXYZRGBA>::Ptr convexHull(new pcl::PointCloud<pcl::PointXYZRGBA>);
    if (regions.size() > 0) {
    	std::cerr << "Number of possible table regions: " << regions.size() << std::endl;
        std::cerr<<"Visualize region"<<std::endl;
    	for (size_t i = 0; i < regions.size(); i++){
    		std::cerr << "Region" << i <<" size: " << regions[i].getCount() << std::endl;
    		pcl::PointCloud<pcl::PointXYZRGBA>::Ptr contour (new pcl::PointCloud<pcl::PointXYZRGBA>);
    		contour->points = regions[i].getContour();

    		if (viewer)
    		{
    			std::stringstream ss; ss << "Region" << i;
	            viewer->removeAllPointClouds();
	            viewer->addPointCloud(contour, ss.str());
	            viewer->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_COLOR, 1.0f, 0.0f, 0.0f,ss.str());
	            viewer->spin();
	            viewer->removeAllPointClouds();
    		}
    	} 

        pcl::PointCloud<pcl::PointXYZRGBA>::Ptr boundary(new pcl::PointCloud<pcl::PointXYZRGBA>);
        boundary->points = regions[0].getContour();
        
        pcl::ConvexHull<pcl::PointXYZRGBA> hull;
        hull.setInputCloud(boundary);
        // Make sure that the resulting hull is bidimensional.
        hull.setDimension(2);
        hull.reconstruct(*convexHull);
    }
    else std::cerr << "Failed to segment Table.\n";
    return convexHull;
}

#endif
