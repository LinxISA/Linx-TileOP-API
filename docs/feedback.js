(() => {
  const link = document.getElementById('feedback-page');
  const site = 'https://linxisa.github.io/Linx-TileOP-API/';

  function updateFeedback() {
    let target;
    try {
      target = document.getElementById(decodeURIComponent(location.hash.slice(1)));
    } catch {
      target = null;
    }
    const article = target?.closest('article') || document.getElementById('home');
    const pageName = article.id === 'home' ? '操作总览' : article.dataset.title;
    const reference = new URL(site);
    reference.hash = target?.closest('article') ? target.id : article.id;
    const issue = new URL('https://github.com/LinxISA/Linx-TileOP-API/issues/new');
    issue.searchParams.set('title', `[TileOP 文档][0.59] ${pageName}：`);
    issue.searchParams.set('body', [
      `手册版本：0.59`,
      `接口或章节：${pageName}`,
      `页面链接：${reference.href}`,
      '',
      '### 问题或建议',
      '',
      '### 建议改法或复现示例（可选）',
      '',
    ].join('\n'));
    link.href = issue.href;
  }

  window.addEventListener('hashchange', updateFeedback);
  updateFeedback();
})();
