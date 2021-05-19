/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2020, Bielefeld University
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Bielefeld University nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

#include "stages.h"
#include <moveit/python/task_constructor/properties.h>
#include <moveit/task_constructor/stages.h>
#include <moveit/task_constructor/stages/pick.h>
#include <moveit/task_constructor/stages/simple_grasp.h>
#include <moveit_msgs/PlanningScene.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace moveit::task_constructor;
using namespace moveit::task_constructor::stages;

namespace moveit {
namespace python {

namespace {

/// extract from python argument a vector<T>, where arg maybe a single T or a list of Ts
template <typename T>
std::vector<T> elementOrList(const py::object& arg) {
	try {
		return std::vector<T>{ arg.cast<T>() };
	} catch (const py::cast_error&) {
		return arg.cast<std::vector<T>>();
	}
}

}  // anonymous namespace

void export_stages(pybind11::module& m) {
	// clang-format off
	properties::class_<ModifyPlanningScene, Stage>(m, "ModifyPlanningScene")
		.def(py::init<const std::string&>(), py::arg("name") = std::string("modify planning scene"))
		.def("attachObject", &ModifyPlanningScene::attachObject)
		.def("detachObject", &ModifyPlanningScene::detachObject)
		.def("attachObjects", [](ModifyPlanningScene& self, const py::object& names,
		                         const std::string& attach_link, bool attach) {
			self.attachObjects(elementOrList<std::string>(names), attach_link, attach);
		}, py::arg("names"), py::arg("attach_link"), py::arg("attach") = true)
		.def("detachObjects", [](ModifyPlanningScene& self, const py::object& names,
		                         const std::string& attach_link) {
			self.attachObjects(elementOrList<std::string>(names), attach_link, false);
		}, py::arg("names"), py::arg("attach_link"))
		.def("allowCollisions", [](ModifyPlanningScene& self,
	        const py::object& first, const py::object& second, bool enable_collision) {
			self.allowCollisions(elementOrList<std::string>(first), elementOrList<std::string>(second), enable_collision);
		}, py::arg("first"), py::arg("second"), py::arg("enable_collision") = true);
	// clang-format on

	properties::class_<CurrentState, Stage>(m, "CurrentState")
	    .def(py::init<const std::string&>(), py::arg("name") = std::string("current state"));

	properties::class_<FixedState, Stage>(m, "FixedState")
	    .def(py::init<const std::string&>(), py::arg("name") = std::string("fixed state"))
#if 0
		.def("setState", [](FixedState& stage, const moveit_msg::PlanningScene& scene_msg) {
			// TODO: How to initialize the PlanningScene?
			planning_scene::PlanningScenePtr scene;
			scene->setPlanningSceneMsg(scene_msg);
			stage.setState(scene);
		})
#endif
	    ;

	properties::class_<ComputeIK, Stage>(m, "ComputeIK")
	    .property<std::string>("eef")
	    .property<std::string>("group")
	    .property<std::string>("default_pose")
	    .property<uint32_t>("max_ik_solutions")
	    .property<bool>("ignore_collisions")
	    .property<geometry_msgs::PoseStamped>("ik_frame")
	    .property<geometry_msgs::PoseStamped>("target_pose")
	    // methods of base class boost::python::class_ need to be called last!
	    .def(py::init<const std::string&, Stage::pointer&&>());

	properties::class_<MoveTo, PropagatingEitherWay, PyMoveTo<>>(m, "MoveTo")
	    .property<std::string>("group")
	    .property<geometry_msgs::PoseStamped>("ik_frame")
	    .property<moveit_msgs::Constraints>("path_constraints")
	    .def(py::init<const std::string&, const solvers::PlannerInterfacePtr&>())
	    .def("setGoal", py::overload_cast<const geometry_msgs::PoseStamped&>(&MoveTo::setGoal))
	    .def("setGoal", py::overload_cast<const geometry_msgs::PointStamped&>(&MoveTo::setGoal))
	    .def("setGoal", py::overload_cast<const moveit_msgs::RobotState&>(&MoveTo::setGoal))
	    .def("setGoal", py::overload_cast<const std::map<std::string, double>&>(&MoveTo::setGoal))
	    .def("setGoal", py::overload_cast<const std::string&>(&MoveTo::setGoal));

	properties::class_<MoveRelative, PropagatingEitherWay, PyMoveRelative<>>(m, "MoveRelative")
	    .property<std::string>("group")
	    .property<geometry_msgs::PoseStamped>("ik_frame")
	    .property<double>("min_distance")
	    .property<double>("max_distance")
	    .property<moveit_msgs::Constraints>("path_constraints")
	    .def(py::init<const std::string&, const solvers::PlannerInterfacePtr&>())
	    .def("setDirection", py::overload_cast<const geometry_msgs::TwistStamped&>(&MoveRelative::setDirection))
	    .def("setDirection", py::overload_cast<const geometry_msgs::Vector3Stamped&>(&MoveRelative::setDirection))
	    .def("setDirection", py::overload_cast<const std::map<std::string, double>&>(&MoveRelative::setDirection));

	py::enum_<stages::Connect::MergeMode>(m, "MergeMode")
	    .value("SEQUENTIAL", stages::Connect::MergeMode::SEQUENTIAL)
	    .value("WAYPOINTS", stages::Connect::MergeMode::WAYPOINTS);
	PropertyConverter<stages::Connect::MergeMode>();

	properties::class_<Connect, Stage>(m, "Connect")
	    .def(py::init<const std::string&, const Connect::GroupPlannerVector&>(),
	         py::arg("name") = std::string("connect"), py::arg("planners"));

	properties::class_<FixCollisionObjects, Stage>(m, "FixCollisionObjects")
	    .property<double>("max_penetration")
	    .def(py::init<const std::string&>(), py::arg("name") = std::string("fix collisions"));

	properties::class_<GenerateGraspPose, MonitoringGenerator>(m, "GenerateGraspPose")
	    .property<std::string>("object")
	    .property<std::string>("eef")
	    .property<std::string>("pregrasp")
	    .property<std::string>("grasp")
	    .property<double>("angle_delta")
	    .def(py::init<const std::string&>());

	properties::class_<GeneratePose, MonitoringGenerator>(m, "GeneratePose")
	    .property<geometry_msgs::PoseStamped>("pose")
	    .def(py::init<const std::string&>());

	properties::class_<Pick, Stage>(m, "Pick")
	    .property<std::string>("object")
	    .property<std::string>("eef")
	    .property<std::string>("eef_frame")
	    .property<std::string>("eef_group")
	    .property<std::string>("eef_parent_group")
	    .def(py::init<Stage::pointer&&, const std::string&>(), py::arg("grasp_generator"),
	         py::arg("name") = std::string("pick"))

	    .def("setApproachMotion", &Pick::setApproachMotion)
	    .def("setLiftMotion",
	         py::overload_cast<const geometry_msgs::TwistStamped&, double, double>(&Pick::setLiftMotion))
	    .def("setLiftMotion", py::overload_cast<const std::map<std::string, double>&>(&Pick::setLiftMotion));

	properties::class_<Place, Stage>(m, "Place")
	    .property<std::string>("object")
	    .property<std::string>("eef")
	    .property<std::string>("eef_frame")
	    .property<std::string>("eef_group")
	    .property<std::string>("eef_parent_group")
	    .def(py::init<Stage::pointer&&, const std::string&>(), py::arg("place_generator"),
	         py::arg("name") = std::string("place"));

	properties::class_<SimpleGrasp, Stage>(m, "SimpleGrasp")
	    .property<std::string>("eef")
	    .property<std::string>("object")
	    .def(py::init<Stage::pointer&&, const std::string&>(), py::arg("pose_generator"),
	         py::arg("name") = std::string("grasp generator"))
	    .def<void (SimpleGrasp::*)(const geometry_msgs::PoseStamped&)>("setIKFrame", &SimpleGrasp::setIKFrame)
	    .def<void (SimpleGrasp::*)(const Eigen::Isometry3d&, const std::string&)>("setIKFrame", &SimpleGrasp::setIKFrame)
	    .def<void (SimpleGrasp::*)(const std::string&)>("setIKFrame", &SimpleGrasp::setIKFrame);

	properties::class_<SimpleUnGrasp, Stage>(m, "SimpleUnGrasp")
	    .property<std::string>("eef")
	    .property<std::string>("object")
	    .def(py::init<Stage::pointer&&, const std::string&>(), py::arg("pose_generator"),
	         py::arg("name") = std::string("place generator"));
}
}  // namespace python
}  // namespace moveit
