module.exports = {
  branches: ['main'],
  plugins: [
    '@semantic-release/commit-analyzer', // 分析 commit 类型
    '@semantic-release/release-notes-generator', // 自动生成 changelog
    '@semantic-release/changelog',
    '@semantic-release/git', // 提交 changelog 和版本号变更
    '@semantic-release/github' // 发布 GitHub Release
  ],
  preset: 'conventionalcommits',
};
