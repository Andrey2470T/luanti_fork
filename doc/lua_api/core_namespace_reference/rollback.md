Rollback
========

* `core.rollback_get_node_actions(pos, range, seconds, limit)`:
  returns `{{actor, pos, time, oldnode, newnode}, ...}`
    * Find who has done something to a node, or near a node
    * `actor`: `"player:<name>"`, also `"liquid"`.
* `core.rollback_revert_actions_by(actor, seconds)`: returns
  `boolean, log_messages`.
    * Revert latest actions of someone
    * `actor`: `"player:<name>"`, also `"liquid"`.
