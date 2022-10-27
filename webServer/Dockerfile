# 基于 JDK8
FROM openjdk:8
# 作者
MAINTAINER MUTOU
# 时区设置
ENV TZ=Asia/Shanghai
# 添加jar包到远程容器中
ADD target/webServer-0.0.1-SNAPSHOT.jar /webServer/webServer.jar
ADD application.yml /webServer/application.yml
# 暴露接口
EXPOSE 8478
# 切换工作路径
WORKDIR /webServer/
# 启动
CMD ["java", "-jar", "webServer.jar"]