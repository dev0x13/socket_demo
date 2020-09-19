#pragma once

// We set this to 65507, because this is maximum data length for a single UDP datagram.
// This is done for simplicity sake, since transferring sequential data through UDP requires
// ARQ protocol implementation and significantly increases the complexity of the code and this
// particular test task.
// TCP doesn't have such a limitation, but we still use this limit for uniformity.
#define MAX_MESSAGE_LENGTH_BYTES 65507