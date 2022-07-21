#include <stdexcept>
#include <string>

namespace vtrs{

class RuntimeError : public std::runtime_error {

private: // *** Private members *** //
    int m_code = 0;
    int m_kind = 0;

public: // *** Public members *** //

    /**
     * Runtime error types.
     *
     * A combination of error type and error kind can
     * determine the type and cause of an error.
     */
    enum ErrorKind : int {
        E_TYPE_VK_RESULT = 120
    };

    /**
     * Defines a runtime exception.
     *
     * @param message The error message.
     * @param kind Error kind from {@link ErrorKind} enum.
     * @param code The error code, defaults to 0.
     */
    RuntimeError(std::string&, ErrorKind, int);

    /**
     * Returns the error code.
     *
     * @return The error code.
    */
    int getCode();

    /**
     * Returns the error kind.
     *
     * @return The error kind.
    */
    int getKind();
};

} // namespace vtrs
