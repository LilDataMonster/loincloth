module.exports = {
  siteMetadata: {
    siteTitle: `Lil Data Monster - Nightgown`,
    defaultTitle: `Lil Data Monster - Nightgown`,
    siteTitleShort: `Nightgown`,
    siteDescription: `The Lil Data Monster Nightgown software stack`,
    siteUrl: `https://lildatamonster.github.io/nightgown/`,
    siteAuthor: `David Fan`,
    siteImage: `/banner.png`,
    siteLanguage: `en`,
    themeColor: `#cf2ccb`,
    basePath: `/`,
    footer: `Lil Data Monster`,
  },
  // prefix path for github pages
  pathPrefix: "/nightgown",
  plugins: [
    {
      resolve: `@rocketseat/gatsby-theme-docs`,
      options: {
        configPath: `src/config`,
        docsPath: `src/docs`,
        githubUrl: `https://github.com/LilDataMonster/nightgown`,
        baseDir: `docs`,
      },
    },
    {
      resolve: `gatsby-plugin-manifest`,
      options: {
        name: `Nightgown`,
        short_name: `Nightgown`,
        start_url: `/`,
        background_color: `#ffffff`,
        display: `standalone`,
        icon: `static/favicon.png`,
      },
    },
    `gatsby-plugin-sitemap`,
    {
      resolve: `gatsby-plugin-google-analytics`,
      options: {
        // trackingId: ``,
      },
    },
    {
      resolve: `gatsby-plugin-canonical-urls`,
      options: {
        siteUrl: `https://lildatamonster.github.io/nightgown/`,
      },
    },
    `gatsby-plugin-offline`,
  ],
};
