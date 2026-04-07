/*
 * InfiniteConveyorPlugin.cc
 * ─────────────────────────────────────────────────────────────────────────
 * Ignition Gazebo Fortress (ign-gazebo 6) system plugin that drives N
 * independent slat models along a prismatic joint at a constant belt speed
 * and, when a slat's world-X position exceeds end_x, teleports it back by
 * loop_length so the belt appears infinite.
 *
 * Runtime control: subscribe to /conveyor/cmd_vel (ignition.msgs.Double)
 * to change belt speed/direction while simulation is running.
 *   positive value = +X direction
 *   negative value = -X direction
 *   zero = stop
 */

#include <cmath>
#include <mutex>
#include <string>
#include <vector>

#include <sdf/Element.hh>

#include <ignition/plugin/Register.hh>

#include <ignition/gazebo/System.hh>
#include <ignition/gazebo/Entity.hh>
#include <ignition/gazebo/EntityComponentManager.hh>
#include <ignition/gazebo/EventManager.hh>

#include <ignition/gazebo/components/Name.hh>
#include <ignition/gazebo/components/Joint.hh>
#include <ignition/gazebo/components/JointPosition.hh>
#include <ignition/gazebo/components/JointPositionReset.hh>
#include <ignition/gazebo/components/JointVelocityCmd.hh>

#include <ignition/common/Console.hh>
#include <ignition/transport/Node.hh>
#include <ignition/msgs/double.pb.h>

using namespace ignition;
using namespace ignition::gazebo;

// ─────────────────────────────────────────────────────────────────────────────
class InfiniteConveyor
  : public ignition::gazebo::System,
    public ignition::gazebo::ISystemConfigure,
    public ignition::gazebo::ISystemUpdate
{
public:

  // ── Configure ─────────────────────────────────────────────────────────────
  void Configure(
    const ignition::gazebo::Entity &,
    const std::shared_ptr<const sdf::Element> &_sdf,
    ignition::gazebo::EntityComponentManager &,
    ignition::gazebo::EventManager &) override
  {
    auto readDouble = [&](const std::string &key, double def) -> double {
      return _sdf->HasElement(key) ? _sdf->Get<double>(key) : def;
    };
    auto readInt = [&](const std::string &key, int def) -> int {
      return _sdf->HasElement(key) ? _sdf->Get<int>(key) : def;
    };

    beltSpeed_   = readDouble("belt_speed",   0.5);
    endCoord_    = readDouble("end_x",        1.05);
    numSlats_    = readInt   ("num_slats",     10);
    slatPitch_   = readDouble("slat_pitch",   0.10);
    firstSlatCoord_ = readDouble("first_slat_x", 0.04);

    // Read optional coordinate from SDF (x, y, or z), default: x
    coordinate_ = "x";
    if (_sdf->HasElement("coordinate"))
      coordinate_ = _sdf->Get<std::string>("coordinate");

    // Read optional slat prefix from SDF, default: "slat"
    slatPrefix_ = "slat";
    if (_sdf->HasElement("slat_prefix"))
      slatPrefix_ = _sdf->Get<std::string>("slat_prefix");

    // Read optional topic name from SDF, default: /conveyor/cmd_vel
    std::string topic = "/conveyor/cmd_vel";
    if (_sdf->HasElement("cmd_vel_topic"))
      topic = _sdf->Get<std::string>("cmd_vel_topic");

    loopLength_ = numSlats_ * slatPitch_;
    startCoord_ = firstSlatCoord_ - slatPitch_;  // wrap-back position

    initialCoord_.resize(numSlats_);
    for (int i = 0; i < numSlats_; ++i)
      initialCoord_[i] = firstSlatCoord_ + i * slatPitch_;

    // Subscribe to speed command topic
    node_.Subscribe(topic, &InfiniteConveyor::OnCmdVel, this);

    ignmsg << "[InfiniteConveyor] prefix=" << slatPrefix_
           << " coord=" << coordinate_
           << " speed=" << beltSpeed_
           << " m/s  loop=" << loopLength_
           << " m  slats=" << numSlats_
           << "  topic=" << topic << "\n";
  }

  // ── Topic callback ────────────────────────────────────────────────────────
  void OnCmdVel(const ignition::msgs::Double &_msg)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    beltSpeed_ = _msg.data();
    ignmsg << "[InfiniteConveyor] Speed changed to: " << beltSpeed_ << " m/s\n";
  }

  // ── Update (every simulation step) ────────────────────────────────────────
  void Update(
    const ignition::gazebo::UpdateInfo &_info,
    ignition::gazebo::EntityComponentManager &_ecm) override
  {
    if (_info.paused) return;

    // Thread-safe read of belt speed
    double currentSpeed;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      currentSpeed = beltSpeed_;
    }

    // ── First-time: discover joint entities ──────────────────────────────────
    if (!initialized_)
    {
      for (int i = 0; i < numSlats_; ++i)
      {
        const std::string jName = slatPrefix_ + "_" + std::to_string(i) + "_joint";
        ignition::gazebo::Entity jEnt = ignition::gazebo::kNullEntity;

        _ecm.Each<components::Joint, components::Name>(
          [&](const ignition::gazebo::Entity &e,
              const components::Joint *,
              const components::Name *n) -> bool
          {
            if (n->Data() == jName) { jEnt = e; return false; }
            return true;
          });

        if (jEnt == ignition::gazebo::kNullEntity)
        {
          ignwarn << "[InfiniteConveyor] joint '" << jName << "' not found yet.\n";
          return;
        }

        if (!_ecm.Component<components::JointPosition>(jEnt))
          _ecm.CreateComponent(jEnt, components::JointPosition());

        if (!_ecm.Component<components::JointVelocityCmd>(jEnt))
          _ecm.CreateComponent(jEnt,
            components::JointVelocityCmd({currentSpeed}));

        jointEntities_.push_back(jEnt);
      }

      ignmsg << "[InfiniteConveyor] All " << numSlats_ << " joints found.\n";
      initialized_ = true;
    }

    // ── Per-slat update ──────────────────────────────────────────────────────
    for (int i = 0; i < (int)jointEntities_.size(); ++i)
    {
      const ignition::gazebo::Entity e = jointEntities_[i];

      // 1. Maintain belt velocity
      auto *vel = _ecm.Component<components::JointVelocityCmd>(e);
      if (vel)
        vel->Data()[0] = currentSpeed;

      // 2. Read joint position
      const auto *pos = _ecm.Component<components::JointPosition>(e);
      if (!pos || pos->Data().empty()) continue;

      const double jp     = pos->Data()[0];
      const double worldCoord = initialCoord_[i] + jp;

      // 3. Wrap: handle both positive and negative directions
      bool needWrap = false;
      double newJp = jp;

      if (currentSpeed >= 0 && worldCoord > endCoord_)
      {
        // Moving in forward direction: wrap back by loop_length
        newJp = jp - loopLength_;
        needWrap = true;
      }
      else if (currentSpeed < 0 && worldCoord < startCoord_)
      {
        // Moving in reverse direction: wrap forward by loop_length
        newJp = jp + loopLength_;
        needWrap = true;
      }

      if (needWrap)
      {
        auto *reset = _ecm.Component<components::JointPositionReset>(e);
        if (!reset)
          _ecm.CreateComponent(e, components::JointPositionReset({newJp}));
        else
          reset->Data() = {newJp};
      }
    }
  }

private:
  double beltSpeed_  {0.5};
  double endCoord_   {1.05};
  double startCoord_ {-0.06};  // wrap-back for reverse direction
  int    numSlats_   {10};
  double slatPitch_  {0.10};
  double firstSlatCoord_ {0.04};
  double loopLength_ {1.0};
  std::string slatPrefix_ {"slat"};
  std::string coordinate_ {"x"};

  std::vector<double>                   initialCoord_;
  std::vector<ignition::gazebo::Entity> jointEntities_;
  bool initialized_{false};

  // Transport for runtime speed control
  ignition::transport::Node node_;
  std::mutex mutex_;
};

// ─────────────────────────────────────────────────────────────────────────────
IGNITION_ADD_PLUGIN(
  InfiniteConveyor,
  ignition::gazebo::System,
  ignition::gazebo::ISystemConfigure,
  ignition::gazebo::ISystemUpdate)

IGNITION_ADD_PLUGIN_ALIAS(InfiniteConveyor, "conveyor::InfiniteConveyor")