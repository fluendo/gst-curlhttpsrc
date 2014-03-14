<xsl:stylesheet
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:fo="http://www.w3.org/1999/XSL/Format"
    version="1.0">

<xsl:output method="xml" version="1.0" indent="yes"/>

<xsl:param name="prefix"></xsl:param>
<xsl:param name="booktitle"></xsl:param>
<xsl:param name="bookpart"></xsl:param>

<xsl:template match="/">
  <book title="ONEPLAY: {$booktitle}"
        name="oneplay-{$bookpart}"
	link="html/index.html">
  <chapters>
    <xsl:apply-templates select="doxygenindex/compound[@kind='group']"/>
  </chapters>
  <functions>
    <xsl:apply-templates select="doxygenindex/compound[@kind='group']/member[@kind='function']"/>
  </functions>
  </book>
</xsl:template>

<xsl:template match="compound[@kind='group']">
  <xsl:param name="name"><xsl:value-of select="name"/></xsl:param>
  <xsl:param name="refid"><xsl:value-of select="@refid"/></xsl:param>
  <xsl:param name="link"><xsl:value-of select="$refid"/>.html</xsl:param>
  <sub name="{$name}" link="html/{$link}"/>
</xsl:template>

<xsl:template match="member">
  <xsl:param name="name"><xsl:value-of select="name"/></xsl:param>
  <xsl:param name="refid"><xsl:value-of select="@refid"/></xsl:param>
  <xsl:param name="before"><xsl:value-of select="substring-before($refid,'_1')"/></xsl:param>
  <xsl:param name="after"><xsl:value-of select="substring-after($refid,'_1')"/></xsl:param>
  <xsl:param name="link"><xsl:value-of select="$before"/>.html#<xsl:value-of select="$after"/></xsl:param>
  <function name="{$name}" link="html/{$link}"/>
</xsl:template>

</xsl:stylesheet>
