// Copyright 2014-2019 Josh Pieper, jjp@pobox.com.  All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <optional>

#include "sophus/se3.hpp"

#include "mjlib/base/visitor.h"

#include "base/point3d.h"

namespace mjmech {
namespace mech {

struct QuadrupedState {
  // The joint level.
  struct Joint {
    int id = 0;

    // These are the raw values reported by the actuator and are not
    // referenced to any particular frame.
    double angle_deg = 0.0;
    double velocity_dps = 0.0;
    double torque_Nm = 0.0;

    double temperature_C = 0.0;
    double voltage = 0.0;
    int32_t mode = 0;
    int32_t fault = 0;

    template <typename Archive>
    void Serialize(Archive* a) {
      a->Visit(MJ_NVP(id));
      a->Visit(MJ_NVP(angle_deg));
      a->Visit(MJ_NVP(velocity_dps));
      a->Visit(MJ_NVP(torque_Nm));
      a->Visit(MJ_NVP(temperature_C));
      a->Visit(MJ_NVP(voltage));
      a->Visit(MJ_NVP(mode));
      a->Visit(MJ_NVP(fault));
    }
  };

  std::vector<Joint> joints;

  struct Link {
    // The topmost link is relative to the "Body" frame.  Each
    // subsequent link is relative to the previous.  The "child" frame
    // references the endpoint of this link.
    Sophus::SE3d pose_child_parent;

    // Each of the these velocities and torques is referenced to the
    // canonical frame for that joint.
    double angle_deg = 0.0;
    double velocity_dps = 0.0;
    double torque_Nm = 0.0;

    // Random diagnostics for this joint.
    int id = 0;

    template <typename Archive>
    void Serialize(Archive* a) {
      a->Visit(MJ_NVP(pose_child_parent));
      a->Visit(MJ_NVP(angle_deg));
      a->Visit(MJ_NVP(velocity_dps));
      a->Visit(MJ_NVP(torque_Nm));
      a->Visit(MJ_NVP(id));
    }
  };

  // The leg end-effector level.
  struct Leg {
    int leg = 0;
    base::Point3D position_mm;
    base::Point3D velocity_mm_s;
    base::Point3D force_N;
    bool stance = false;

    std::vector<Link> links;

    template <typename Archive>
    void Serialize(Archive* a) {
      a->Visit(MJ_NVP(leg));
      a->Visit(MJ_NVP(position_mm));
      a->Visit(MJ_NVP(velocity_mm_s));
      a->Visit(MJ_NVP(force_N));
      a->Visit(MJ_NVP(stance));
      a->Visit(MJ_NVP(links));
    }

    friend Leg operator*(const Sophus::SE3d&, const Leg& rhs);
  };

  std::vector<Leg> legs_B;

  // And finally, the robot level.
  struct Robot {
    Sophus::SE3d pose_mm_LR;
    Sophus::SE3d pose_mm_RB;
    base::Point3D v_mm_s_LB;  // velocity
    base::Point3D w_LB;  // angular rate

    template <typename Archive>
    void Serialize(Archive* a) {
      a->Visit(MJ_NVP(pose_mm_LR));
      a->Visit(MJ_NVP(pose_mm_RB));
      a->Visit(MJ_NVP(v_mm_s_LB));
      a->Visit(MJ_NVP(w_LB));
    }
  };

  Robot robot;

  // Now the state associated with various control modes.  Each mode's
  // data is only valid while that mode is active.

  struct StandUp {
    enum Mode {
      kPrepositioning,
      kStanding,
      kDone,
    };

    static inline std::map<Mode, const char*> ModeMapper() {
      return {
        { kPrepositioning, "prepositioning" },
        { kStanding, "standing" },
        { kDone, "done" },
      };
    };

    Mode mode = kPrepositioning;

    struct Leg {
      int leg = 0;
      base::Point3D pose_mm_R;

      template <typename Archive>
      void Serialize(Archive* a) {
        a->Visit(MJ_NVP(leg));
        a->Visit(MJ_NVP(pose_mm_R));
      }
    };

    std::vector<Leg> legs;

    template <typename Archive>
    void Serialize(Archive* a) {
      a->Visit(MJ_ENUM(mode, ModeMapper));
      a->Visit(MJ_NVP(legs));
    }
  };

  StandUp stand_up;

  template <typename Archive>
  void Serialize(Archive* a) {
    a->Visit(MJ_NVP(joints));
    a->Visit(MJ_NVP(legs_B));
    a->Visit(MJ_NVP(robot));

    a->Visit(MJ_NVP(stand_up));
  }
};

inline QuadrupedState::Leg operator*(const Sophus::SE3d& pose_AB,
                                     const QuadrupedState::Leg& rhs_B) {
  auto result_A = rhs_B;
  result_A.position_mm = pose_AB * rhs_B.position_mm;
  result_A.velocity_mm_s = pose_AB.so3() * rhs_B.velocity_mm_s;
  result_A.force_N = pose_AB.so3() * rhs_B.force_N;
  return result_A;
}

}
}
