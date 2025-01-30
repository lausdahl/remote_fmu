//
// Created by Guldbrandt Lausdahl, Kenneth on 10/01/2025.
//
#include "Status.h"

namespace rfmu {
    const Status &Status::OK = Status();
    const Status &Status::CANCELLED = Status(StatusCode::CANCELLED, "");
}
