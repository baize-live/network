package live.baize.controller.exceptionHandler;

import live.baize.controller.responseResult.CodeEnum;
import live.baize.controller.responseResult.MsgEnum;
import live.baize.controller.responseResult.Result;
import live.baize.service.util.MailUtil;
import lombok.extern.slf4j.Slf4j;
import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.bind.annotation.RestControllerAdvice;

@Slf4j
@RestControllerAdvice
public class BaiZeExceptionHandler {

    @ExceptionHandler(BusinessException.class)
    public Result handleBusinessException(BusinessException businessException) {
        // 直接返回即可
        return new Result(businessException.getCode(), null, businessException.getMessage());
    }

    @ExceptionHandler(SystemException.class)
    public Result handleSystemException(SystemException systemException) {
        // 记录日志
        // 发送信息给运维
        // 发送信息给开发人员
        MailUtil.sendMailToSelf(systemException.getMessage());
        // 返回前端信息
        return new Result(systemException.getCode(), null, systemException.getMessage());
    }

    @ExceptionHandler(Exception.class)
    public Result handleException(Exception exception) {
        // 记录日志

        // 发送信息给运维

        // 发送信息给开发人员
        MailUtil.sendMailToSelf(exception.getMessage());
        // 返回前端信息
        return new Result(CodeEnum.SYSTEM_UNKNOWN, null, MsgEnum.SYSTEM_UNKNOWN);
    }

}
