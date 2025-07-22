
# 비기너 커리큘럼 과제

## 업로드 방식
1. fork
브라우저로 이동
[https://github.com/SII-git/beginner-heap](https://github.com/SII-git/begginer-heap)
우측 상단 Fork 버튼 클릭
→ 내 계정에 저장소 복사됨
예: `https://github.com/[자기 ID]/beginner-heap`
2. 로컬 과제 폴더에 Git 초기화
```bash
cd [과제 폴더]
git init
```
3. 원격 저장소 연결
```bash
# origin: 내가 Fork한 저장소
git remote add origin https://github.com/[자기 ID]/beginner-heap.git
```
4. 자기 이름 브랜치 생성
```bash
git checkout -b [자기 ID]
```
예: `git checkout -b hong1234`
5. 과제 파일 업로드 및 커밋
```bash
git add .
git commit -m "[커밋 메시지]"
```
6. fork한 저장소에 push
```bash
git push origin [자기 ID]
```
예: `git push origin hong1234`
7. github 에서 pull request 생성 
브라우저에서 your-id/beginner-heap 저장소로 이동

"Compare & pull request" 버튼 클릭

다음 조건으로 PR 생성:
- base repository: SII-git/begginer-heap
- base branch: \[자기 ID\]
- compare: \[자기 ID\]:\[자기 ID\]

## 다른 사람꺼 확인
다른 사람 브랜치 확인
```bash
git clone https://github.com/SII-git/begginer-heap.git
git checkout [다른 사람 ID]
```
다른 사람 태그 확인
```bash
git clone https://github.com/SII-git/begginer-heap.git
git checkout tags/[다른 사람 ID]-[version]
```

