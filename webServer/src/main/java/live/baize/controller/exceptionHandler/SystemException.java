package live.baize.controller.exceptionHandler;

import lombok.Getter;

@Getter
public class SystemException extends RuntimeException {
    private final Integer code;

    public SystemException(Integer code, String message) {
        super(message);
        this.code = code;
    }

    public SystemException(Integer code, String message, Throwable cause) {
        super(message, cause);
        this.code = code;
    }
}
