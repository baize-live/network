package live.baize.controller.responseResult;

import lombok.Data;
import lombok.NoArgsConstructor;
import lombok.experimental.Accessors;

@Data
@NoArgsConstructor
@Accessors(chain = true)
public class Result {
    private String msg;
    private Object data;
    private Integer code;

    public Result(Integer code, Object data, String msg) {
        this.msg = msg;
        this.data = data;
        this.code = code;
    }
}
