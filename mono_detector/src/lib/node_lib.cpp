/*
  multi_sensor_calibration
  Copyright (C) 2019 Intelligent Vehicles, Delft University of Technology

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "mono_detector/node_lib.hpp"

#include <pcl/common/transforms.h>
#include <pcl_conversions/pcl_conversions.h>

#include <memory>
#include <string>

#include <ament_index_cpp/get_package_share_directory.hpp>

#include "mono_detector/detector.hpp"
#include "mono_detector/util.hpp"
#include "mono_detector/yaml.hpp"

using std::placeholders::_1;

namespace mono_detector {

using PointCloud2 = sensor_msgs::msg::PointCloud2;
using Image = sensor_msgs::msg::Image;
using CameraInfo = sensor_msgs::msg::CameraInfo;

MonoDetectorNode::MonoDetectorNode() : Node("mono_detector") {
  RCLCPP_INFO(get_logger(), "Initialized mono detector.");
  std::string package_share = ament_index_cpp::get_package_share_directory("mono_detector");

  // Load object points from ros parameter server
  this->declare_parameter("object_points_file", package_share + "/config/object_points.yaml");
  std::string object_points_file = this->get_parameter("object_points_file")
    .get_parameter_value().get<std::string>();
  object_points_ = YAML::LoadFile(object_points_file).as<std::vector<cv::Point3f>>();

  cv::Point3f center = calculateCenter(object_points_);
  std::sort(object_points_.begin(), object_points_.end(), [center](cv::Point3f a, cv::Point3f b) {
    return std::atan((a.y - center.y) / (a.x - center.x)) >
           std::atan((b.y - center.y) / (b.x - center.x));
  });

  // Load configuration from file
  this->declare_parameter("yaml_file", package_share + "/config/image_processing.yaml");
  std::string yaml_file = this->get_parameter("yaml_file").get_parameter_value().get<std::string>();
  config_ = YAML::LoadFile(yaml_file).as<mono_detector::Configuration>();

  auto qos = rclcpp::SensorDataQoS();
  // Setup subscriber and publisher
  image_subscriber_       = this->create_subscription<Image>(
    "image_raw", qos, std::bind(&MonoDetectorNode::imageCallback, this, _1)
  );
  camera_info_subscriber_ = this->create_subscription<CameraInfo>(
    "camera_info", qos, std::bind(&MonoDetectorNode::cameraInfoCallback, this, _1)
  );
  point_cloud_publisher_  = this->create_publisher<PointCloud2>("mono_pattern", 100);
}

void MonoDetectorNode::imageCallback(Image::ConstSharedPtr const & in) {
  RCLCPP_INFO_ONCE(get_logger(), "Receiving images.");
  if (!intrinsics_received_) {
    return;
  }

  try {
    // Call to do image processing
    std::vector<cv::Point2f> image_points;
    Eigen::Isometry3f isometry = detectMono(toOpencv(in), config_, intrinsics_);

    // Transform pattern
    pcl::PointCloud<pcl::PointXYZ> transformed_pattern;
    pcl::transformPointCloud(toPcl(object_points_), transformed_pattern, isometry);

    // Publish pattern
    RCLCPP_DEBUG_THROTTLE(get_logger(), *get_clock(), 1000,
      "Detected a mono detector pattern point cloud.");
    PointCloud2 out;
    pcl::toROSMsg(transformed_pattern, out);
    out.header = in->header;
    point_cloud_publisher_->publish(out);
  } catch (DetectionException & e) {
    RCLCPP_DEBUG_STREAM_THROTTLE(get_logger(), *get_clock(), 1000,
      "Detection failed: '" << e.what() << "'.");
  } catch (std::exception & e) {
    RCLCPP_ERROR_STREAM(get_logger(), "Exception thrown: '" << e.what() << "'.");
  }
}

void MonoDetectorNode::cameraInfoCallback(CameraInfo const & camera_info) {
  RCLCPP_INFO_ONCE(get_logger(), "Receiving camera info.");
  // ToDo: Use message filters to make sure to get both camera info and image
  intrinsics_.fromCameraInfo(camera_info);
  intrinsics_received_ = true;
}

}  // namespace mono_detector
