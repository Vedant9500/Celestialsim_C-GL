#include "core/Particle.h"

namespace nbody {

Particle::Particle(const glm::vec2& pos, const glm::vec2& vel, float m)
    : position(pos)
    , velocity(vel)
    , acceleration(0.0f, 0.0f)
    , mass(m)
{
}

Particle::Particle(const glm::vec2& pos, const glm::vec2& vel, const glm::vec2& acc, float m)
    : position(pos)
    , velocity(vel)
    , acceleration(acc)
    , mass(m)
{
}

void Particle::SetVelocity(const glm::vec2& vel) {
    velocity = vel;
}

void Particle::SetPosition(const glm::vec2& pos) {
    position = pos;
}

void Particle::SetAcceleration(const glm::vec2& acc) {
    acceleration = acc;
}

} // namespace nbody
