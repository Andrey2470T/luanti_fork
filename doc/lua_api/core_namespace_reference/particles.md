Particles
=========

* `core.add_particle(particle definition)`
    * Deprecated: `core.add_particle(pos, velocity, acceleration,
      expirationtime, size, collisiondetection, texture, playername)`

* `core.add_particlespawner(particlespawner definition)`
    * Add a `ParticleSpawner`, an object that spawns an amount of particles
      over `time` seconds.
    * Returns an `id`, and -1 if adding didn't succeed
    * Deprecated: `core.add_particlespawner(amount, time,
      minpos, maxpos,
      minvel, maxvel,
      minacc, maxacc,
      minexptime, maxexptime,
      minsize, maxsize,
      collisiondetection, texture, playername)`

* `core.delete_particlespawner(id, player)`
    * Delete `ParticleSpawner` with `id` (return value from
      `core.add_particlespawner`).
    * If playername is specified, only deletes on the player's client,
      otherwise on all clients.
