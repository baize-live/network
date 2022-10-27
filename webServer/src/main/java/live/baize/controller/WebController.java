package live.baize.controller;


import live.baize.bean.Person;
import live.baize.controller.responseResult.CodeEnum;
import live.baize.controller.responseResult.MsgEnum;
import live.baize.controller.responseResult.Result;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/server")
public class WebController {

    @PostMapping
    public Result postHelloWorld(Person person) {
        return new Result(CodeEnum.SYSTEM_UNKNOWN, person, MsgEnum.RES_POST);
    }

    @GetMapping
    public Result getHelloWorld(Person person) {
        return new Result(CodeEnum.SYSTEM_UNKNOWN, person, MsgEnum.RES_GET);
    }
}
