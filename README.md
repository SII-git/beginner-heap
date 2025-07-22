
# 비기너 커리큘럼 과제

## 업로드 방식

1. 로컬 과제 폴더에 Git 초기화
```bash
cd [과제 폴더]
git init
```
2. 원격 저장소 연결
```bash
git remote add origin https://github.com/SII-git/begginer-heap.git
```
3. 새 브랜치로 작업
```bash
git checkout -b [자기 ID]
```
예: `git checkout -b hong1234`
4. 과제 파일 업로드 및 커밋
```bash
git add .
git commit -m "[커밋 메시지]"
```
5. 원격 브랜치 푸시
```bash
git push origin [자기 ID]
```
예: `git push origin hong1234`
6. tag 생성 [optional]
```bash
git tag [자기 ID]-[version]
git push origin [자기 ID]-[version]
```

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

