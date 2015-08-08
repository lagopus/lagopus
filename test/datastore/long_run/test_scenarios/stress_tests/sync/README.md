<!-- -*- mode: markdown -*- -->

Stress tests for DataStore (sync).
===================================

* 01_test_allobj_repetition_100.yml  
  This is a test "create => enable => disable => destroy" for all objects.
  Will repeat this 100 times.

* 02_test_allobj_create_100.yml  
  This is a test "create => enable" for all objects.
  Will repeat this 100 times.

* 03_test_atomic_commit_repetition_100.yml  
  This is a test "atomic begin => create (all objects) =>
  bridge enable => atomic commit => destroy-all-obj".
  Will repeat this 100 times.

* 04_test_atomic_abort_repetition_100.yml  
  This is a test "atomic begin => create (all objects) =>
  bridge enable => atomic abort".
  Will repeat this 100 times.

* 05_test_save_load_repetition_100.yml  
  This is a test "create (all objects) => bridge enable =>
  save => destroy-all-obj => load => destroy-all-obj"
  Will repeat this 100 times.

* 06_test_interface_port_repetition_100.yml  
  This is a test set/unset interface in port.
  Will repeat this 100 times.

* 07_test_channel_controller_repetition_100.yml  
  This is a test set/unset channel in controller.
  Will repeat this 100 times.

* 08_test_controller_bridge_repetition_100.yml  
  This is a test set/unset controller in bridge.
  Will repeat this 100 times.

* 09_test_port_bridge_repetition_100.yml  
  This is a test set/unset port in bridge.
  Will repeat this 100 times.

* 10_test_connect_ryu_repetition_100.yml  
  This is a test connect/disconnect ryu.
  Will repeat this 100 times.

* 11_test_connect_bridge_enable_repetition_100.yml  
  This is a test "enable => disable" for bridge.
  Will repeat this 100 times.

* 12_test_allobj_show_repetition_100.yml  
  This is a test show for all objects.
  Will repeat this 100 times.

* 13_test_interface_fail_repetition_100.yml  
  This is a test "create => create (same name)" for interface.
  Will repeat this 100 times.

* 14_test_port_fail_repetition_100.yml  
  This is a test "create => create (same name)" for port.
  Will repeat this 100 times.

* 15_test_channel_fail_repetition_100.yml  
  This is a test "create => create (same name)" for channel.
  Will repeat this 100 times.

* 16_test_controller_fail_repetition_100.yml  
  This is a test "create => create (same name)" for controller.
  Will repeat this 100 times.

* 17_test_bridge_fail_repetition_100.yml  
  This is a test "create => create (same name)" for bridge.
  Will repeat this 100 times.
