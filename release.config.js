module.exports = {
  branches: ['main'],
  plugins: [
    '@semantic-release/commit-analyzer', // 分析 commit 类型
    '@semantic-release/release-notes-generator', // 自动生成 changelog
    '@semantic-release/github' // 发布 GitHub Release
  ],
  preset: 'conventionalcommits',
};
