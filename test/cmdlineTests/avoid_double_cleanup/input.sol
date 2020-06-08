// SPDX-License-Identifier: GPLv3

contract C {
  function f() external {
        bytes memory amount;
        bytes24 zero_bytes24;
        bytes32 signature_root;
        bytes32 node = sha256(abi.encodePacked(
            sha256(abi.encodePacked(amount, zero_bytes24, signature_root))
        ));
  }
}
