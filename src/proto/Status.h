//
// Created by Guldbrandt Lausdahl, Kenneth on 09/01/2025.
//

#ifndef STATUS_H
#define STATUS_H
#include <string>

namespace rfmu {
    enum StatusCode {
        /// Not an error; returned on success.
        OK = 0,

        /// The operation was cancelled (typically by the caller).
        CANCELLED = 1,

        UNKNOWN = 2,


        ABORTED = 10,


        INTERNAL = 13,


        /// Force users to include a default branch:
        DO_NOT_USE = -1
    };

    class Status {
    public:
        Status() : code_(StatusCode::OK) {
        }

        Status(StatusCode code, const std::string &error_message)
            : code_(code), error_message_(error_message) {
        }

        static const Status &OK;
        /// A CANCELLED pre-defined instance.
        static const Status &CANCELLED;

        [[nodiscard]] StatusCode error_code() const { return code_; }
        /// Return the instance's error message.
        [[nodiscard]] std::string error_message() const { return error_message_; }
        /// Return the (binary) error details.
        // Usually it contains a serialized google.rpc.Status proto.
        [[nodiscard]] std::string error_details() const { return binary_error_details_; }

        /// Is the status OK?
        bool ok() const { return code_ == StatusCode::OK; }

        void IgnoreError() const {
        }

    private:
        StatusCode code_;
        std::string error_message_;
        std::string binary_error_details_;
    };
} // namespace rfmu

#endif //STATUS_H
