import OpenAI from "openai";
import { Octokit } from "@octokit/rest";
import { execSync } from "node:child_process";

const openai = new OpenAI({
  apiKey: process.env.OPENAI_API_KEY,
});

const octokit = new Octokit({
  auth: process.env.GITHUB_TOKEN,
});

const [owner, repo] = process.env.REPOSITORY.split("/");
const pullNumber = Number(process.env.PR_NUMBER);

if (!process.env.OPENAI_API_KEY) {
  throw new Error("OPENAI_API_KEY secret is missing.");
}

const { data: pr } = await octokit.pulls.get({
  owner,
  repo,
  pull_number: pullNumber,
});

execSync(`git fetch origin ${pr.base.ref}:refs/remotes/origin/${pr.base.ref}`, {
  stdio: "inherit",
});

execSync(`git fetch origin pull/${pullNumber}/head:pr-${pullNumber}`, {
  stdio: "inherit",
});

execSync(`git checkout pr-${pullNumber}`, {
  stdio: "inherit",
});

const diff = execSync(`git diff --unified=80 origin/${pr.base.ref} HEAD`, {
  encoding: "utf8",
  maxBuffer: 20 * 1024 * 1024,
});

if (!diff.trim()) {
  await octokit.issues.createComment({
    owner,
    repo,
    issue_number: pullNumber,
    body: "Codex review: 변경된 diff가 없습니다.",
  });
  process.exit(0);
}

const response = await openai.responses.create({
  model: "gpt-5.2-codex",
  reasoning: {
    effort: "medium",
  },
  input: [
    {
      role: "system",
      content: `
너는 시니어 코드 리뷰어다.
PR diff를 보고 실제 문제가 될 수 있는 것만 리뷰한다.

찾을 것:
- 버그
- 보안 문제
- 런타임 에러
- 깨지는 엣지 케이스
- 누락된 테스트
- 데이터 손상 가능성
- 성능상 큰 문제

하지 말 것:
- 단순 취향
- 코드 스타일 잔소리
- 근거 없는 추측
- "좋습니다" 같은 의미 없는 말

한국어 Markdown으로 답해라.
문제가 없으면 "뚜렷한 문제를 찾지 못했습니다."라고 말해라.
`,
    },
    {
      role: "user",
      content: `다음 GitHub PR diff를 리뷰해줘:\n\n${diff}`,
    },
  ],
});

const reviewBody = `## Codex PR Review

${response.output_text}
`;

await octokit.issues.createComment({
  owner,
  repo,
  issue_number: pullNumber,
  body: reviewBody,
});
