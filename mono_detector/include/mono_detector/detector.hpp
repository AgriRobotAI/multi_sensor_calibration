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

#pragma once

#include <vector>

#include <opencv2/opencv.hpp>

#include "mono_detector/types.hpp"
#include "mono_detector/util.hpp"

namespace mono_detector {

/// Function to calculate the location of the calibration board in the world
Eigen::Isometry3f detectMono(
  cv::Mat const & image,
  Configuration const & configuration,
  CameraModel const & intrinsics
);

}  // namespace mono_detector
