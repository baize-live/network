package live.baize.controller;


import live.baize.controller.responseResult.CodeEnum;
import live.baize.controller.responseResult.Result;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/server")
public class WebController {

    @PostMapping
    public Result postHelloWorld() {
        // 返回值
        return new Result(CodeEnum.SYSTEM_UNKNOWN, "post请求 hello world", "你好");
    }

    @GetMapping
    public Result getHelloWorld() {
        // 返回值
        return new Result(CodeEnum.SYSTEM_UNKNOWN, "get请求 hello world", "你好");
    }
}
