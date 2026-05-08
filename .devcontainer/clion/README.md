# Pintos CLion 개발 가이드

CLion은 Dev Container로 열지 말고 저장소 루트를 로컬 프로젝트로 엽니다.
IDE는 로컬에서 돌고, 빌드/실행만 Docker 컨테이너에서 수행합니다.

CLion용 설정도 VSCode와 같은 `.devcontainer/Dockerfile`을 사용합니다.
컨테이너 내부 경로는 `/workspaces/pintos_22.04_lab_docker`입니다.

## 준비

- Docker Desktop
- CLion

처음 한 번 이미지를 빌드합니다.

```bash
make image
```

## Make로 빌드/채점

테스트 통과 여부를 볼 때는 `result` 타깃을 사용합니다. 이 방식은 `.output`,
`.errors`, `.result`를 만들고 `PASS`/`FAIL`을 판정합니다.
이 문서는 `make` 기준으로 사용합니다. `pintos` 명령을 직접 실행할 수도 있지만,
직접 실행하면 빌드 여부, 실행 경로, Docker 포트 매핑, 테스트 판정을 따로 맞춰야
해서 실수하기 쉽습니다.

```bash
make threads
make threads-result TEST=alarm-zero
```

`threads-result`는 테스트 후 자동 종료됩니다. `threads-run`은 `-q` 없이 실행되므로
`PASS` 후 QEMU가 남을 수 있습니다.

네 프로젝트는 `PROJECT`와 `TEST`를 바꿔 실행합니다.

```bash
make result PROJECT=threads TEST=alarm-zero
make result PROJECT=userprog TEST=args-none
make result PROJECT=vm TEST=pt-grow-stack
make result PROJECT=filesys TEST=base/sm-create
```

컨테이너 셸이 필요하면:

```bash
make shell PROJECT=threads
```

## 디버깅

```bash
make threads-gdb TEST=alarm-zero
```

그 다음 CLion에서 `Pintos Remote Debug`를 Debug로 실행해 attach합니다.
재실행 전에 남은 컨테이너는 아래 `1234` 포트 정리 명령으로 제거합니다
(원래는 `make` 실행 터미널에서 QEMU를 끄는 게 좋지만, 현재는 시도해도 종료되지
않아 컨테이너를 정리합니다).

`threads-gdb`는 디버거가 붙을 때까지 QEMU를 멈춰두는 용도라 테스트 후 자동 종료를
기대하지 않습니다. 채점만 할 때는 `threads-result`를 사용합니다.

## 1234 포트가 이미 사용 중일 때

이전 GDB 컨테이너가 남아 있으면 `port is already allocated`가 납니다.

확인 없이 바로 정리하려면:

```bash
containers=$(docker ps -aq --filter "publish=1234" --filter "ancestor=pintos-dev:22.04"); [ -z "$containers" ] || docker rm -f $containers
```

이 명령은 `1234` 포트를 쓰는 모든 프로세스를 지우는 게 아니라, `pintos-dev:22.04`
이미지로 뜬 Docker 컨테이너 중 `1234` 포트를 publish한 것만 제거합니다. 그래서
이전 Pintos GDB 컨테이너만 정리하면서 다른 Docker 컨테이너를 건드릴 가능성을
줄입니다.

확인 후 지우려면:

```bash
docker ps -a --filter "publish=1234" --filter "ancestor=pintos-dev:22.04" --format "table {{.ID}}\t{{.Names}}\t{{.Status}}\t{{.Ports}}"
docker rm -f <CONTAINER_ID>
```

macOS에서 포트 점유만 확인하려면:

```bash
lsof -nP -iTCP:1234 -sTCP:LISTEN
```

## CLion Remote Debug 설정

- `Debugger`: `GDB`
- `Target`: `tcp:127.0.0.1:1234`
- `Symbol file`: `$PROJECT_DIR$/pintos/threads/build/kernel.o`
